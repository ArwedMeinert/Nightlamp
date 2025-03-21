# Nightlamp
Small project where I used some spare neopixels to create a nightlamp that can store three colors.
The colors can be picked using the digital encoder, the color that is changed can be choosen with a press on the encoder (RGB). First, the color that should be changed needs to be chosen with the three buttons, then it needs to be changed. It can be saved by holding down the same button as before until the light has dimmed down. Pressing the button short will either change the color to the one saved by this button or turn the lamp off (depending on if the color is active at the moment).

## PCB
All electric connections were done on a prototyping board. However, the connections are not too hard to track back with the code (four buttons, two encoder signals, neopixel data signal). 

<img src="https://github.com/user-attachments/assets/c0ccf8d7-334c-4197-b974-9cfa66704438" width="200" title="CAD model of the lamp"/>
<img src="https://github.com/user-attachments/assets/d6b40923-22af-413b-b1fa-0efdb5278178" width="200" title="Real lamp turned off"/>

<img src="https://github.com/user-attachments/assets/7f51d6e2-39b2-49df-bb4c-692e4ec7a33a" width="200" title="Real lamp turned on" />
