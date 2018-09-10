# intel-msr-voltages
Experimental application for setting voltage offsets on Intel CPUs.

## Disclaimer
I am not responsible for any damage you cause by following the instructions below.

This method of setting voltage offsets has not been thoroughly tested.

There is no official documentation for the model-specific register (MSR) interface.

All public knowledge on the MSR `0x150` can be found in the [linux-intel-undervolt github project](https://github.com/mihic/linux-intel-undervolt)

## Prerequisites
- Intel CPU(s) based on Haswell or subsequent microarchitectures
- Linux Operating System
- Linux Kernel Module `msr`
- [msr-tools](https://github.com/01org/msr-tools)
- [systemd](https://www.freedesktop.org/wiki/Software/systemd/)
- [gcc](http://gcc.gnu.org/)
- [GNU make](http://www.gnu.org/software/make)

## Obtaining the source code
```
git clone git://github.com/mikhail-j/intel-msr-voltages.git
```

## Overvolting/Undervolting Information
As [tiziw](https://github.com/tiziw/iuvolt) suggests, more information can be found at [mihic's linux-intel-undervolt github project](https://github.com/mihic/linux-intel-undervolt) on voltage planes.

The voltage offsets can either be positive or negative numbers.

## Build the application
Run `make` to build the application.

## Cleanup the previous build
Run `make clean` to cleanup the previous build of the application.

This is not required if `make` was successful (no errors during compilation).

## Testing Voltage Offsets
By default, the application will try to read the voltage offsets from `/etc/intel-msr-voltages.conf` in millivolt (mV) units.

The voltage offset configuration can be set by copying the new voltage offsets in `intel-msr-voltages.conf`:
```
sudo cp intel-msr-voltages.conf /etc/
```

After copying the new voltage offset configuration, you can run the `intel-msr-voltages` executable.
```
sudo ./intel-msr-voltages
```

## Installing the application
Both the `intel-msr-voltages` executable and `intel-msr-voltages.conf` configuration file will be installed.

```
sudo make install
```

Note: The systemd service status can be checked by running:
```
sudo systemctl status intel-msr-voltages
```

## Uninstalling the application
```
sudo make uninstall
```

## Start the service
If the service has not started, run:
```
sudo systemctl start intel-msr-voltages
```

## References
- [mihic's linux-intel-undervolt github project](https://github.com/mihic/linux-intel-undervolt)
- [tiziw's iuvolt github project](https://github.com/tiziw/iuvolt)