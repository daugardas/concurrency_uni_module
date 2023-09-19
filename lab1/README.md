# Compiliation and running
To compile the `lab1.cpp` file and create an executable named `lab1`, run the following command in the terminal:

    g++ -o lab1 lab1.cpp

If there are no errors, a new executable file named `lab1` will be created in the same directory. To launch the executable, run the following command:

    ./lab
This will execute the `lab1` program and any output will be displayed in the terminal window.

Note: if you are compiling `lab1-2.cpp`, you need to add `-fopenmp` flag to the compiler:
    
    g++ -o lab1-2 lab1-2.cpp -fopenmp