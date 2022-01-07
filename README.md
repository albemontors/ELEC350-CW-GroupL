# ELEC35X Coursework C1

This is the starter code for ELEC350/351 C1. You should use the ARMC6 compiler to build this code in Mbed Studio.

**A pre-requisite for this is to complete the lab on networking https://github.com/UniversityOfPlymouth-Electronics/Embedded-Systems/blob/master/level6/network_programming.md**

This includes watching all the videos on setting up Microsoft Azure IoT Central.


## Using the Module Support Board

Demo code has now been integrated into the module support board library.

In main.cpp, you can uncomment the following two lines to test your board:

```C++
//UOP_MSB_TEST  board;  //This class is purely for testing. Do no use it otherwise!!!!!!!!!!!
//board.test();         //Look inside here to see how this works
```

If you look inside the class member function `board.test()` you will see commented examples of how to use various aspects of the module support board, including the SD card.

*Don't forget to comment our or remove these two lines before you write your own code!*

