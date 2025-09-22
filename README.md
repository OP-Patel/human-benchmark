# human-benchmark
De1-SoC implementation of the popular Human Benchmark Memory Game (https://humanbenchmark.com/tests/memory).

### Main Skills: <br>
> De1-SoC FPGA interfacing to use I/O devices (PS/2 Keyboard, VGA display, Audio Speakers), C Programming

### Report

ECE243 Report: “Human Benchmark: Memory Game”

What is our project?
Our project tests the memory capabilities of the user by showing a random number of white squares on a grid for ~5 seconds on the VGA, then hides these squares and allows the user to use PS/2 keyboard to traverse the grid in order to select the squares that originally were flashed white. Each level gives the user 3 chances to select an incorrect square, after 3 chances are used up on the level a life is deducted and a new set of white squares are flashed for the level. Once your lives reach 0, the game is now over. 

Detailed Walkthrough:

Preliminary: Obtain the source code, compile and run it. *Please ensure the PS/2 keyboard, VGA display, and speakers are connected. 

Controls: Use WASD to move and ENTER to submit. *Using WASD will make a lower pitch beep, and pressing enter will make a high pitch beep. 

1. At this point, you should see a difficulty selection menu (see Figure 1).  Use “A” and “D” keys to move through the menu and press enter on the difficulty you wish to start at: we suggest you start with easy. 

*The only difference between easy, medium and hard is the starting grid size: Easy - 3x3, Medium - 4x4, Hard - 5x5

<img width="506" height="373" alt="image" src="https://github.com/user-attachments/assets/80ce9ae8-3b90-41e6-b0d4-4fdc9d52b56a" />

Figure 1. Difficulty selection menu 


2. After selecting a difficulty, you should see a grid with certain squares in white (see Figure 2). These are the squares you need to remember, they will only be shown for 5 seconds before disappearing. Couple things to note, you have 3 chances per level, and 3 lives per game. These are indicated top-right of the screen. 

<img width="515" height="391" alt="image" src="https://github.com/user-attachments/assets/864c5002-6307-4457-aae7-2541d27af925" />

Figure 2. White squares shown for Level 1 Easy Mode. 

3. Now in the selection phase, you will see a crosshair serving as the cursor. You can move this cursor by using WASD, and can select a square you think was white by pressing enter. If you select correctly, the white square will be revealed. However, if you select incorrectly, you will lose a chance. See Figure 3 below. Note, selecting an already picked white square will not result in a penalty, but selecting the same incorrect square again will result in a deduction of your chances. 

<img width="1021" height="381" alt="image" src="https://github.com/user-attachments/assets/d86449a0-553d-49a2-86c1-81c6e133b10d" />

Figure 3. Selecting an incorrect square (left). Selecting the correct square (right). 

4. After selecting all the correct squares you will progress to the next level. However, if you use up all your chances without selecting all the white squares. The level will restart with a different pattern of white squares; you will also lose 1 life. 

5. Higher levels will have a larger grid and more squares to remember, Figure 4 shows Level 3. 

<img width="482" height="380" alt="image" src="https://github.com/user-attachments/assets/2a5b7548-01b6-47c9-8744-521e6e23401b" />

Figure 4. Level 3 on Easy Mode. 

6.  After losing all your lives you will see a “GAME OVER” screen, see Figure 5. Here, you can press enter to restart back at the difficulty menu selection.


<img width="501" height="372" alt="image" src="https://github.com/user-attachments/assets/d0007b55-7e66-45fa-9538-71b05137f302" /> <br>
Figure 5. Game over screen.

<br>

## Block Diagram: <br> 
<img width="560" height="620" alt="image" src="https://github.com/user-attachments/assets/053317f4-bdc4-405d-9566-dfc3f989d7c0" />

