/*
 * intel-msr-voltages.h
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

#include <stdbool.h>
#include <stdint.h>

#define INTEL_MSR_VOLTAGE_VERSION "1.0.0"
#define INTEL_MSR_VOLTAGE_CONFIGURATION_FILE_PATH "/etc/intel-msr-voltages.conf"

bool is_intel_cpu();

bool msr_tools_exists();

int rdmsr_msr_voltage(int32_t* voltage_value);

int32_t compute_voltage_hexadecimal(double d);

int wrmsr_write_voltage_offset(int voltage_plane_index, int32_t offset);

int wrmsr_read_voltage_offset(int voltage_plane_index);

bool is_valid_voltage_string(char* voltage);

int read_voltage_offset_configuration(char* filepath, double voltages[]);

int set_voltage_offset_configuration(double voltages[]);

void usage(void);
