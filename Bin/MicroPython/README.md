# MicroPython for PicoCalc

Here is pre-compiled uf2 files for micropython running on PicoCalc based on [PicoCalc-micropython-driver](https://github.com/zenodante/PicoCalc-micropython-driver)

- micropython_pico.uf2
- micropython_pico2.uf2  
- micropython_pico2w.uf2 

These uf2 files already included main.py ,so just put into PicoCalc , you will get a python running terminal immediately   
No need to use Thonny , for those who interest to try python.  

For developers, just visit [PicoCalc-micropython-driver](https://github.com/zenodante/PicoCalc-micropython-driver)  

Follow the readme to  
- Flash firmware and upload python code through Thonny IDE. 
- Compile custom micropython firmware for picocalc.

---------   
Here is the Sequence for beginners to run python on Pico with PicoCalc:  

1. Put pico into **BOOTSEL** mode.
2. Flash correct firmware uf2 file that included/compiled all necessary python modules.
3. Open Thonny at the mean time, config Interpreter with correct kind and port (usuall /dev/ttyACM0 on linux).  
4. Upload main.py( this is the entry boot up python file) and other py files to /. from left sidebar.
5. Do nothing but wait for uploading finished,then unplug the Micro-usb cable from pico.
6. Plug USB-C cable into PicoCalc and Power On to see if everything goes right.

here is the sample screenshots about thonny:   

![interpreter](https://github.com/clockworkpi/PicoCalc/blob/master/wiki/micropython_thonny_config.png)

![micropython](https://github.com/clockworkpi/PicoCalc/blob/master/wiki/micropython_thonny.png)



