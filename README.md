# RasPiLEDControl

Control SK6812 LED strips with a Raspberry Pi. The initial goal
was to have sunrise alarm clock playing a sunrise animation at a 
given time.

Of course this is also the base framework for any other use case
you need to control a LED stripe over the local Wifi.

If you want a matching  Android frontend have a look at
https://github.com/kiozen/LEDAmbiente

## Dependencies

* ASIO ( >1.19) - https://think-async.com/
* FMT (> 8.0.0)- https://fmt.dev/latest/index.html
* WS281x - https://github.com/jgarff/rpi_ws281x
* nlohmann (>3.9) JSON - https://github.com/nlohmann/json
* sigslot (v1.2.0) - https://github.com/palacaze/sigslot
* libcrypt

## Build System

* GCC 8.3
* C++17
* CMake

# Build

## Build Dependencies

These are the instructions to build the project on a Raspberry Pi Zero W. 
It is assumed that you have installed latest Raspbian OS. A headless 
installation will do. Wifi and ssh is expected to work.

This is just the list of commands I did to setup a plain system. As I 
am fed up with messing around with outdated Debian packages I 
installed the dependencies from scratch. You might disagree - in 
that case do whatever you think is best.

```
sudo apt-get install build-essential git cmake libssl-dev 
mkdir tmp
cd tmp

wget https://sourceforge.net/projects/asio/files/asio/1.20.0%20%28Stable%29/asio-1.20.0.tar.gz
tar xvf asio-1.20.0.tar.gz
cd asio-1.20.0/
./configure --prefix=/usr
sudo make install-data
cd ..

wget https://github.com/fmtlib/fmt/releases/download/8.1.0/fmt-8.1.0.zip
mkdir build_fmt
cd build_fmt
cmake -DCMAKE_INSTALL_PREFIX=/usr -DFMT_TEST=OFF - ../fmt-8.1.0
sudo make install
cd ..

git clone https://github.com/jgarff/rpi_ws281x.git
mkdir build_rpi_ws281x
cd build_rpi_ws281x/
cmake -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TEST=OFF ../rpi_ws281x/
sudo make install
cd ..

git clone https://github.com/nlohmann/json.git
mkdir build_json
cmake -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF -DJSON_BuildTests=OFF ../json
sudo make install
cd ..

git clone https://github.com/palacaze/sigslot.git
mkdir build_sigslot
cmake -DCMAKE_INSTALL_PREFIX=/usr -DSIGSLOT_COMPILE_EXAMPLES=OFF -DSIGSLOT_COMPILE_TESTS=OFF  ../sigslot
sudo make install
cd ../../

rm -rf tmp
```

## Build LEDControl

Now it's time to compile LEDControl itself. It takes quite some time
on the Raspberry Pi Zero W. If you really want to jump into development
you probably want to install a cross compiler on your PC and copy the
binary over. But for just using it building it on the Raspberry is fine.

```
cd ~
git clone https://github.com/kiozen/RasPiLEDControl.git
mkdir build_ledctrl
cd build_ledctrl
cmake ../RasPiLEDControl/
make
cp bin/ledcontrol ..
cp ../RasPiLEDControl/script/example.json ..
```

The last copy will install an example animation. You probably want to create 
your own ones later.

## Install and prepare the Raspberry

Stop audio output:

```
sudo nano /etc/modprobe.d/blacklist-snd.conf
``` 

And add line:

```
blacklist snd_bcm2835
```

Next the configuration has to be edited:

```
sudo nano /boot/config.txt
```

Search for 
```
# Enable audio (loads snd_bcm2835)
dtparam=audio=on
```

and comment the line:

```
#dtparam=audio=on
```

Now you have to reboot the system.

## First Test

Of course you have to wire up the Raspberry Pi Zero first. For my 300 LED / 5m 
stripe of SK6812 LEDs it was sufficient to connect the data line to pin GPIO18.

I use a 10A power supply that powers the Raspberry Pi, too. If you use an
additional source for the Pi make sure all ground plains are connected.

Now it's time for a first test. Simply start ledcontrol:

```
cd ~
sudo ./ledcontrol
```

On first startup ypu get:
```
22-01-08 14:00:04.182 [D] animation: Found animation 'example C54E0D09AC7A6FF1C41B745FECD2B975'.
22-01-08 14:00:04.247 [D] ctrl: MAC address is B8:27:EB:0A:0B:05
22-01-08 14:00:04.252 [E] ctrl: Parsing config failed: [json.exception.parse_error.101] parse error at line 1, column 1: syntax error while parsing value - unexpected end of input; expected '[', '{', or a literal
22-01-08 14:00:04.255 [D] ctrl: *** start asio loop ***
```

The error is ok for first start. Second start should look like:

```
22-01-08 14:06:45.825 [D] animation: Found animation 'example C54E0D09AC7A6FF1C41B745FECD2B975'.
22-01-08 14:06:45.890 [D] ctrl: MAC address is B8:27:EB:0A:0B:05
22-01-08 14:06:45.899 [I] light: Restored light
22-01-08 14:06:45.901 [I] alarm: Restored alarm
22-01-08 14:06:45.904 [I] ctrl: Restored all configurations
22-01-08 14:06:45.906 [D] ctrl: *** start asio loop ***
```

The device should show up in the GUI app https://github.com/kiozen/LEDAmbiente.
You should be able to switch on lights.

## Start with systemd

In the sources you find a systemd start script.

```
cd ~
sudo cp  RasPiLEDControl/systemd/ledctrl.service /lib/systemd/system/
sudo systemctl enable ledctrl
sudo systemctl start ledctrl
sudo systemctl status ledctrl
```

This should show something like:

```
● ledctrl.service - Control LED stripe
     Loaded: loaded (/lib/systemd/system/ledctrl.service; enabled; vendor preset: enabled)
     Active: active (running) since Sat 2022-01-08 14:10:57 CET; 13s ago
   Main PID: 584 (ledcontrol)
      Tasks: 1 (limit: 415)
        CPU: 609ms
     CGroup: /system.slice/ledctrl.service
             └─584 /home/pi/ledcontrol
```

To follow the log output you can do:

```
journalctl -u ledctrl -f
```







