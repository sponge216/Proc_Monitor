# Process Monitor for Linux

## User Manual

### Setup And Installation
In order to run the program you'll need 3 things:
1) Linux distro with standard unix commands
2) C compiler. Most linux distros already have GCC preinstalled.
3) The ncurses library.

For debian/ubuntu, you can download the ncurses library using this command: sudo apt-get install libncurses5-dev libncursesw5-dev.
For CentOS/RHEL/Scientific Linux 6.x/7.x+ and Fedora Linux 21 or older, use this instead: sudo yum install ncurses-devel.
For Fedora Linux 22.x+, use: sudo dnf install ncurses-devel.

Once you downloaded the ncurses library, clone this repository or download all the files.
In order to clone a repository, use CD to go to your preferred directory. Once there, use this command: git clone https://github.com/sponge216/Proc_Monitor.

After all of these steps, compile the "fullBuild.c" file and make sure to link the ncurses library.
In order to compile the file, use this command: clang/gcc fullBuildDemo.c -o ProcessMonitor -lncurses.
Make sure to change the command according to your compiler.

Now that we've compiled the file, run the program by using: ./ProcessMonitor.


### How To Use
The program will automatically show you all your current running processes.
Use the up/down/left/right arrow keys on your keyboard in order to navigate the menus.
Up/down will scroll your processes list, while left/right is used to choose different options.

The monitor has 4 options:
1) Sorting
2) Filtering
3) Refresh rate
4) Notifications

You can sort your processes according to username, pid, elapsed time, cpu percentage, memory percentage, and command. 
The order of input is important. First enter the option you'd like to sort by, and then select a sign, + for ascending, - for descending.

In order to filter your processes you need to type according to the filter format. The format is: "option:sign:value". You can put multiple filters as long as you separate them by a comma.

You can change your refresh rate per minute by entering a new one. The default is 180 per minute.

Notifications for new processes is currently disabled since it's still in development.

### Design Choices
I decided to implement a scalable design for creating windows and adding future content.
Using the createGeneralMenu function and a functions array, you can create menus that call upon other menus without moving any windows and changing the structure of the program.
The function returns the index of the option chosen, and by that you can call upon another function from the functions array.

I used linked lists to keep track of the processes' information, breaking down every info into its own data member which helps to easily analyze the information for any future features.
Another data structure that I used in this project is a hash set, to handle information about new/old processes and make it easily accessible.

There's a color system that is used to signify information about the processes' data. Currently only red exists, and it means that the process uses more than X amount of cpu/mem percentage.

### Limitations And Future Plans
The current biggest limitation is the GUI, it is very buggy and prone to breaking. Resizing the windows or having a small window will cause the program to malfunction.
My future plan for the project is to debug my notifications system and make it work, as well as improving the GUI and further improve error handling and documentation.

# Conclusion
This project took me a lot of time and effort. I spent many hours on it and I greatly appreciate the opportunity given to me by your company.
This was an incredible learning experience and I hope you may see the effort I put into it. 
I look forward to hearing back from you, as well as letting me know where I can improve.
