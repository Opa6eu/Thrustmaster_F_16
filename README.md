# Thrustmaster_F_16
I wrote this code for the Arduino Pro Micro board. I wanted to convert an old Thrustmaster F16 controller to use it with a PC.

The following components were used in the project:
1. Arduino Pro Micro
2. USB-USB_micro cable
3. CD4021 8-bit shift registers x 3 (already present in the controller)
4. Some buttons (already present in the controller)
5. Potentiometers x 2 (already present in the controller)

However, the controller I received was not working, so I had to replace the shift registers and potentiometers. The project did not turn out to be very successful because the potentiometers had a very limited range of operation. If you plan to replicate this project, I suggest using Hall effect sensors instead of potentiometers, but this will require modifications to the controller's design, which I did not want to do.
