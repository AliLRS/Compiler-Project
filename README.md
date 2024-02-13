<h3>     </h3>
<h1 align="center"> Implementation of a compiler </h1>
<h3 align="center"> Compiler Design Fundamentals </h3>
<h5 align="center"> Final Project - <a href="https://en.sbu.ac.ir/">Shahid Beheshti University</a>(2023) </h5>
<h3>      </h3>


This project focuses on designing and implementing a simple compiler using llvm. To build the project, execute:

```bash
$ ./build.sh
```
To compile your code, replace it with the code in `input.txt` and execute:
```bash
$ ./run.sh
```
This compiler displays the value assigned in each assignment as `The result is:  `.

## Sample

Input:
```
int i = 0;
loopc i < 5: begin
i += 1;
end
```

Output:
```
The result is: 1
The result is: 2
The result is: 3
The result is: 4
The result is: 5
```