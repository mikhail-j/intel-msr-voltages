/*
 * Intel MSR Voltage Offset Tool
 * 
 * Copyright (C) 2018 Qijia (Michael) Jin
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <float.h>
#include <cpuid.h>

//popen()/pclose()/getopt_long()
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#include "intel-msr-voltages.h"

static const struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"version", 0, 0, 'V'},
	{0, 0, 0, 0},
};

static const char short_options[] = "hV";

int main(int argc, char* argv[]) {
	//exit code definitions
	//0 = success
	//1 = unable to set all user specified voltage offsets
	//2 = found non-Intel CPU
	//3 = 'msr' kernel module not loaded
	//4 = no root permissions detected
	//5 = msr-tools not found
	//6 = configuration file not found
	int exit_status = 0;

	int option_char;

	//there are 6 voltage planes
	double voltage_planes[6];

	//unset voltages will be initialized as '-DBL_MAX'
	for (int i = 0; i < 6; i++) {
		voltage_planes[i] = -DBL_MAX;
	}

	while ((option_char = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (option_char) {
			case 'V':
				fprintf(stderr, "intel-msr-voltages version %s\n", INTEL_MSR_VOLTAGE_VERSION);
				exit(0);
			case 'h':
				usage();
				exit(0);
			default:
				fprintf(stderr, "Try 'intel-msr-voltages --help' for more information.\n");
				exit(127);
		}
	}

	if (is_intel_cpu()) {
		if (msr_tools_exists()) {
			if (check_msr_module() == 0) {
				if ((getuid() == 0) && (geteuid() == 0)) {
					int set_config_status = set_voltage_offset_configuration(voltage_planes);
					if (set_config_status == 0) {
						printf("intel-msr-voltages: voltage offset configuration was set successfully!\n");
					}
					else if (set_config_status == -1) {
						exit_status = 6;
					}
					else {
						exit_status = 1;
					}
				}
				else {
					fprintf(stderr, "intel-msr-voltages: error: msr-tools require root permissions!\n");
					exit_status = 4;
				}
			}
			else {
				exit_status = 3;
			}
		}
		else {
			exit_status = 5;
		}
	}
	else {
		exit_status = 2;
	}

	//free allocated variables
	return exit_status;
}

bool is_intel_cpu() {
	char* cpu_signature = malloc(13 * sizeof(char*));
	*(cpu_signature+12) = '\0';		//set null character as end of CPU signature
	bool is_intel;
	unsigned int eax = 0;

	unsigned int cpuid_success = __get_cpuid((unsigned int)0x0000,
											&eax,
											(unsigned int*)cpu_signature,
											(unsigned int*)(cpu_signature+8),
											(unsigned int*)(cpu_signature+4));

	if (!cpuid_success) {
		fprintf(stderr, "intel-msr-voltages: error: cpuid instruction failed!");
		is_intel = false;
	}
	else {
		is_intel = (strcmp(cpu_signature, "GenuineIntel") == 0);
	}

	//free allocated variables
	free(cpu_signature);
	return is_intel;
}

bool msr_tools_exists() {
	char* current_line = malloc(1024 * sizeof(char *));
	bool wrmsr_found;
	bool rdmsr_found;
	int wrmsr_which_popen_exit_code;
	int rdmsr_which_popen_exit_code;

	FILE* which_wrmsr_output = popen("which wrmsr", "r");
	while (fgets(current_line, 1024, which_wrmsr_output) != NULL) {}
	wrmsr_which_popen_exit_code = WEXITSTATUS(pclose(which_wrmsr_output));
	wrmsr_found = (wrmsr_which_popen_exit_code == 0);

	FILE* which_rdmsr_output = popen("which rdmsr", "r");
	while (fgets(current_line, 1024, which_rdmsr_output) != NULL) {}
	rdmsr_which_popen_exit_code = WEXITSTATUS(pclose(which_rdmsr_output));
	rdmsr_found = (rdmsr_which_popen_exit_code == 0);

	//free allocated variables
	free(current_line);

	return (wrmsr_found && rdmsr_found);
}

int rdmsr_msr_voltage(int32_t* voltage_value) {
	char* current_line = malloc(100 * sizeof(char *));
	int32_t previous_msr_value;
	int rdmsr_status = 0;
	int rdmsr_popen_exit_code;

	FILE* rdmsr_output = popen("rdmsr 0x150", "r");
	while (fgets(current_line, 100, rdmsr_output) != NULL) {}
	rdmsr_popen_exit_code = WEXITSTATUS(pclose(rdmsr_output));

	if (rdmsr_popen_exit_code == 0) {
		int scan_status = sscanf(current_line, "%x", voltage_value);
		if (scan_status != 1) {
			rdmsr_status = -2;	//bad hexadecimal input
			*voltage_value = 0xffffffff;
		}
	}
	else {
		fprintf(stderr, "intel-msr-voltages: error: rdmsr requires root permissions!\n");
		rdmsr_status = -1;	//need root permissions
		*voltage_value = 0xffffffff;
	}

	//free allocated variables
	free(current_line);

	return rdmsr_status;
}

int32_t compute_voltage_hexadecimal(double d) {
	double offset_double = d * 1.024;
	int32_t offset_int = round(offset_double);

	//bitshift left by 21
	offset_int = offset_int << 21;

	return offset_int;
}

int wrmsr_write_voltage_offset(int voltage_plane_index, int32_t offset) {
	int wrmsr_status = 0;
	int sprintf_status;

	//popen() command
	char* wrmsr_command = malloc(40 * sizeof(char*));

	//null terminated 64-bit hexadecimal string
	char* wrmsr_input_value = malloc(19 * sizeof(char*));
	wrmsr_input_value[18] = '\0';

	char* current_line = malloc(100 * sizeof(char *));

	sprintf_status = sprintf(wrmsr_input_value, "0x80000%i11%.8x", voltage_plane_index, offset);
	if (sprintf_status < 0) {
		fprintf(stderr, "intel-msr-voltages: error: wrmsr argument value could not be generated!\n");
		wrmsr_status = -1;
	}
	else {
		sprintf_status = sprintf(wrmsr_command, "wrmsr 0x150 %s", wrmsr_input_value);
		if (sprintf_status < 0) {
			fprintf(stderr, "intel-msr-voltages: error: wrmsr command could not be generated!\n");
			wrmsr_status = -2;
		}
		else {
			FILE* wrmsr_output = popen(wrmsr_command, "r");
			while (fgets(current_line, 100, wrmsr_output) != NULL) {}
			int wrmsr_popen_exit_code = WEXITSTATUS(pclose(wrmsr_output));

			if (wrmsr_popen_exit_code != 0) {
				fprintf(stderr, "intel-msr-voltages: error: wrmsr requires root permissions!\n");
				wrmsr_status = -3;	//need root permissions
			}
		}
	}

	//free allocated variables
	free(current_line);
	free(wrmsr_input_value);
	free(wrmsr_command);
	return wrmsr_status;
}

int wrmsr_read_voltage_offset(int voltage_plane_index) {
	int status = 0;
	int sprintf_status;

	//popen() command
	char* wrmsr_command = malloc(40 * sizeof(char*));

	//null terminated 64-bit hexadecimal string
	char* wrmsr_input_value = malloc(19 * sizeof(char*));
	wrmsr_input_value[18] = '\0';

	char* current_line = malloc(100 * sizeof(char *));

	sprintf_status = sprintf(wrmsr_input_value, "0x80000%i10%.8x", voltage_plane_index, (int32_t)0);
	if (sprintf_status < 0) {
		fprintf(stderr, "intel-msr-voltages: error: wrmsr argument value could not be generated!\n");
		status = -1;
	}
	else {
		sprintf_status = sprintf(wrmsr_command, "wrmsr 0x150 %s", wrmsr_input_value);
		if (sprintf_status < 0) {
			fprintf(stderr, "intel-msr-voltages: error: wrmsr command could not be generated!\n");
			status = -2;
		}
		else {
			FILE* wrmsr_output = popen(wrmsr_command, "r");
			while (fgets(current_line, 100, wrmsr_output) != NULL) {}
			int wrmsr_popen_exit_code = WEXITSTATUS(pclose(wrmsr_output));

			if (wrmsr_popen_exit_code != 0) {
				fprintf(stderr, "intel-msr-voltages: error: wrmsr requires root permissions!\n");
				status = -3;	//need root permissions
			}
		}
	}

	//free allocated variables
	free(current_line);
	free(wrmsr_input_value);
	free(wrmsr_command);

	return status;
}

bool is_valid_voltage_string(char* voltage) {
	int digit_count = 0;
	int radix_point_count = 0;
	for (int i=0; i < strlen(voltage); i++) {
		switch(voltage[i]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				++digit_count;
				break;
			case '.':
				++radix_point_count;
				break;
			case '-':
			case '+':
				break;
			default:
				if (!isspace(voltage[i])) {
					return false;
				}
		}
	}

	//it is up to the user to make sure there is no whitespace within the number
	//i.e. 80. 86 instead of 80.86
	return ((digit_count > 0) && (radix_point_count <= 1));
}

int read_voltage_offset_configuration(char* filepath, double voltages[]) {
	int status = 0;
	struct stat configuration_file_status;
	int stat_result = stat(filepath, &configuration_file_status);
	FILE* configuration_file;
	char* current_line = malloc(100 * sizeof(char));

	if (stat_result != 0) {
		fprintf(stderr, "intel-msr-voltages: error: %s could not be found!\n", filepath);
		status = -1;
	}
	else {
		configuration_file = fopen(filepath, "r");
		if (configuration_file != NULL) {
			while ((current_line = fgets(current_line, 100, configuration_file)) != NULL) {
				current_line[strcspn(current_line, "\r\n")] = '\0';		//remove newlines
				if ((current_line[0] == '#') ||
					(strchr(current_line, ':') == NULL) ||
					(strcspn(current_line, "#") < strcspn(current_line, ":"))) {
				}
				else {
					double voltage_value;
					int scan_status;

					size_t key_length = strcspn(current_line, ":");
					char* key_string = malloc((key_length + 1) * sizeof(char));
					size_t value_length = strcspn((current_line + key_length + 1), "#");
					char* value_string = malloc((value_length + 1) * sizeof(char));

					memcpy(value_string, (current_line + key_length + 1), value_length);
					value_string[value_length] = '\0';

					memcpy(key_string, current_line, key_length);
					key_string[key_length] = '\0';

					if (is_valid_voltage_string(value_string)) {
						int scan_status = sscanf(value_string, "%lf", &voltage_value);
						if (scan_status != 1) {
							fprintf(stderr, "intel-msr-voltages: error: failed to read voltage setting for '%s'!\n", key_string);
						}
						else {
							if (strstr(key_string, "cpu_core_voltage_offset") != NULL) {
								voltages[0] = voltage_value;
							}
							else if (strstr(key_string, "intel_gpu_voltage_offset") != NULL) {
								voltages[1] = voltage_value;
							}
							else if (strstr(key_string, "cpu_cache_voltage_offset") != NULL) {
								voltages[2] = voltage_value;
							}
							else if (strstr(key_string, "system_agent_voltage_offset") != NULL) {
								voltages[3] = voltage_value;
							}
							else if (strstr(key_string, "analog_io_voltage_offset") != NULL) {
								voltages[4] = voltage_value;
							}
							else if (strstr(key_string, "digital_io_voltage_offset") != NULL) {
								voltages[5] = voltage_value;
							}
							else {
								fprintf(stderr, "intel-msr-voltages: error: key '%s' not recognized!\n", key_string);
							}
						}
					}

					//free allocated variables
					free(value_string);
					free(key_string);
				}
			}

			//print voltage offset configuration
			for (int i = 0; i < 6; i++) {
				if (voltages[i] != -DBL_MAX) {
					printf("intel-msr-voltages: voltage plane index: %i | voltage offset: %fmV\n", i, voltages[i]);
				}
			}
		}
		else {
			fprintf(stderr, "intel-msr-voltages: configuration file could not be found!\n");
		}
	}
	
	//free allocated variables
	free(current_line);

	return status;
}

int set_voltage_offset_configuration(double voltages[]) {
	int status = 0;
	int32_t* current_msr_voltage_value = malloc(sizeof(int32_t));

	int read_config_status = read_voltage_offset_configuration(INTEL_MSR_VOLTAGE_CONFIGURATION_FILE_PATH, voltages);
	if (read_config_status == 0) {
		for (int i = 0; i < 6; i++) {
			if (voltages[i] != -DBL_MAX) {
				int32_t voltage_hexadecimal = compute_voltage_hexadecimal(voltages[i]);
				
				int wrmsr_write_status = wrmsr_write_voltage_offset(i, voltage_hexadecimal);
				if (wrmsr_write_status == 0) {
					int wrmsr_read_status = wrmsr_read_voltage_offset(i);
					if (wrmsr_read_status == 0) {
						int rdmsr_status = rdmsr_msr_voltage(current_msr_voltage_value);
						if (rdmsr_status == 0) {
							if (*current_msr_voltage_value == voltage_hexadecimal) {
								printf("intel-msr-voltages: voltage offset 0x%.8x was set to voltage plane index %i\n", voltage_hexadecimal, i);
							}
							else {
								fprintf(stderr, "intel-msr-voltages: error: unable to set 0x%.8x to voltage plane index %i\n", voltage_hexadecimal, i);
								status = -5;
							}
						}
						else {
							status = -4;
						}
					}
					else {
						status = -3;
					}
				}
				else {
					status = -2;
				}
			}
		}
	}
	else {
		status = -1;
	}

	//free allocated variables
	free(current_msr_voltage_value);

	return status;
}

void usage() {
	fprintf(stderr,
				"Usage: intel-msr-voltages [OPTION]...\n"
				"Set Intel MSR Voltage Offsets.\n"
				"\n"
				"  -h, --help     Display this help and exit\n"
				"  -V, --version  Output version information and exit\n"
				"\n");
	return;
}

int check_msr_module() {
	int status = 0;
	struct stat msr_module_status;
	int stat_result = stat("/dev/cpu/0/msr", &msr_module_status);
	FILE* load_msr_module_output;
	char* current_line = malloc(100 * sizeof(char));

	//load msr kernel module if possible
	if (stat_result != 0) {
		load_msr_module_output = popen("modprobe msr", "r");
		while (fgets(current_line, 100, load_msr_module_output) != NULL) {}
		int load_msr_module_popen_exit_code = WEXITSTATUS(pclose(load_msr_module_output));

		if (load_msr_module_popen_exit_code != 0) {
			fprintf(stderr, "intel-msr-voltages: error: kernel module 'msr' failed to load!\n");
			status = -1;
		}
	}

	//free allocated variables
	free(current_line);

	return status;
}
