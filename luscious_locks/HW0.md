# Welcome to Homework 0!

For these questions you'll need the mini course and virtual machine (Linux-In-TheBrowser) at -

http://cs-education.github.io/sys/

Let's take a look at some C code (with apologies to a well known song)-
```C
// An array to hold the following bytes. "q" will hold the address of where those bytes are.
// The [] mean set aside some space and copy these bytes into teh array array
char q[] = "Do you wanna build a C99 program?";

// This will be fun if our code has the word 'or' in later...
#define or "go debugging with gdb?"

// sizeof is not the same as strlen. You need to know how to use these correctly, including why you probably want strlen+1

static unsigned int i = sizeof(or) != strlen(or);

// Reading backwards, ptr is a pointer to a character. (It holds the address of the first byte of that string constant)
char* ptr = "lathe"; 

// Print something out
size_t come = fprintf(stdout,"%s door", ptr+2);

// Challenge: Why is the value of away zero?
int away = ! (int) * "";


// Some system programming - ask for some virtual memory

int* shared = mmap(NULL, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
munmap(shared,sizeof(int*));

// Now clone our process and run other programs
if(!fork()) { execlp("man","man","-3","ftell", (char*)0); perror("failed"); }
if(!fork()) { execlp("make","make", "snowman", (char*)0); execlp("make","make", (char*)0)); }

// Let's get out of it?
exit(0);
```

## So you want to master System Programming? And get a better grade than B?
```C
int main(int argc, char** argv) {
	puts("Great! We have plenty of useful resources for you, but it's up to you to");
	puts(" be an active learner and learn how to solve problems and debug code.");
	puts("Bring your near-completed answers the problems below");
	puts(" to the first lab to show that you've been working on this.");
	printf("A few \"don't knows\" or \"unsure\" is fine for lab 1.\n"); 
	puts("Warning: you and your peers will work hard in this class.");
	puts("This is not CS225; you will be pushed much harder to");
	puts(" work things out on your own.");
	fprintf(stdout,"This homework is a stepping stone to all future assignments.\n");
	char p[] = "So, you will want to clear up any confusions or misconceptions.\n";
	write(1, p, strlen(p) );
	char buffer[1024];
	sprintf(buffer,"For grading purposes, this homework 0 will be graded as part of your lab %d work.\n", 1);
	write(1, buffer, strlen(buffer));
	printf("Press Return to continue\n");
	read(0, buffer, sizeof(buffer));
	return 0;
}
```
## Watch the videos and write up your answers to the following questions

**Important!**

The virtual machine-in-your-browser and the videos you need for HW0 are here:

http://cs-education.github.io/sys/

The coursebook:

http://cs241.cs.illinois.edu/coursebook/index.html

Questions? Comments? Use Piazza:
https://piazza.com/illinois/fall2020/cs241

The in-browser virtual machine runs entirely in Javascript and is fastest in Chrome. Note the VM and any code you write is reset when you reload the page, *so copy your code to a separate document.* The post-video challenges (e.g. Haiku poem) are not part of homework 0 but you learn the most by doing (rather than just passively watching) - so we suggest you have some fun with each end-of-video challenge.

HW0 questions are below. Copy your answers into a text document because you'll need to submit them later in the course. More information will be in the first lab.

## Chapter 1

In which our intrepid hero battles standard out, standard error, file descriptors and writing to files.

### Hello, World! (system call style)
1. Write a program that uses `write()` to print out "Hi! My name is `<Your Name>`".

```C
#include <unistd.h>

int main() {
	write(1,"Hi! My name is Jerry Qiu",24);
	return 0;
}
```
### Hello, Standard Error Stream!
2. Write a function to print out a triangle of height `n` to standard error.
   - Your function should have the signature `void write_triangle(int n)` and should use `write()`.
   - The triangle should look like this, for n = 3:
   ```C
   *
   **
   ***
   ```

```C
#include <unistd.h>
void write_triangle(int n);
void write_triangle(int n) {
	int i;
	for (i = n; i; i--) {
		int j;
		for (j = n - i + 1; j; j--) {
			write(STDERR_FILENO, "*", 1);
		}
		write(STDERR_FILENO, "\n", 1);
	}
}
```
### Writing to files
3. Take your program from "Hello, World!" modify it write to a file called `hello_world.txt`.
   - Make sure to to use correct flags and a correct mode for `open()` (`man 2 open` is your friend).

```C
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
	char *s = "Hi! My name is Jerry Qiu";
	mode_t mode = S_IRUSR | S_IWUSR;
	int fildes = open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
	write(fildes, s, strlen(s) + 1);
	close(fildes);
	return 0;
}
```
### Not everything is a system call
4. Take your program from "Writing to files" and replace `write()` with `printf()`.
   - Make sure to print to the file instead of standard out!

```C
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
	char *s = "Hi! My name is Jerry Qiu";
	mode_t mode = S_IRUSR | S_IWUSR;
	close(1);
	int fildes = open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
	printf("%s\n", s);
	close(fildes);
	return 0;
}
```
5. What are some differences between `write()` and `printf()`?

prinf() can be considered as a function that convert the data into a sequence of bytes and that calls write() to write those bytes onto the output, 
while write() can only write a sequence of bytes.


## Chapter 2

Sizing up C types and their limits, `int` and `char` arrays, and incrementing pointers

### Not all bytes are 8 bits?
1. How many bits are there in a byte?

8 bits

2. How many bytes are there in a `char`?

1 byte

3. How many bytes the following are on your machine?
   - `int`, `double`, `float`, `long`, and `long long`

   4 bytes, 8 bytes, 4 bytes, 8 bytes, 8bytes

### Follow the int pointer
4. On a machine with 8 byte integers:
```C
int main(){
    int data[8];
} 
```
If the address of data is `0x7fbd9d40`, then what is the address of `data+2`?
0x7fbd9d48

5. What is `data[3]` equivalent to in C?
   - Hint: what does C convert `data[3]` to before dereferencing the address?

   *(data+3)

### `sizeof` character arrays, incrementing pointers
  
Remember, the type of a string constant `"abc"` is an array.

6. Why does this segfault?
```C
char *ptr = "hello";
*ptr = 'J';
```
ptr here is read only

7. What does `sizeof("Hello\0World")` return?

12

8. What does `strlen("Hello\0World")` return?

5

9. Give an example of X such that `sizeof(X)` is 3.

char X[3] = {'a','b','c'};

10. Give an example of Y such that `sizeof(Y)` might be 4 or 8 depending on the machine.

long Y = 48;

## Chapter 3

Program arguments, environment variables, and working with character arrays (strings)

### Program arguments, `argc`, `argv`
1. What are two ways to find the length of `argv`?

length of argv is argc, 
or we can use a while loop and iterate from argv[0] to the null pointer to count the length

2. What does `argv[0]` represent?

./program

### Environment Variables
3. Where are the pointers to environment variables stored (on the stack, the heap, somewhere else)?

the pointers to environment variables are stored above the stack.

### String searching (strings are just char arrays)
4. On a machine where pointers are 8 bytes, and with the following code:
```C
char *ptr = "Hello";
char array[] = "Hello";
```
What are the values of `sizeof(ptr)` and `sizeof(array)`? Why?

sizeof(ptr) is 4 because ptr is the address which has 4 bytes,
sizeof(array) is 6 because it has 5 char and a 0 at the end of the string.

### Lifetime of automatic variables

5. What data structure manages the lifetime of automatic variables?

stack

## Chapter 4

Heap and stack memory, and working with structs

### Memory allocation using `malloc`, the heap, and time
1. If I want to use data after the lifetime of the function it was created in ends, where should I put it? How do I put it there?

heap; use malloc(x) to reserve some memory location on the heap where x is the number of bytes.

2. What are the differences between heap and stack memory?

In a stack, the allocation and deallocation is automatically done, 
in heap, it needs to be done by the programmer manually.
Stack is not flexible, the memory size allotted cannot be altered, 
heap is flexible, and the allotted memory can be changed.

3. Are there other kinds of memory in a process?

Yes, there are BSS (Uninitialized data segment),
DS (Initialized data segment),
and Text.

4. Fill in the blank: "In a good C program, for every malloc, there is a ___".

free

### Heap allocation gotchas
5. What is one reason `malloc` can fail?

not enough memory to allocate

6. What are some differences between `time()` and `ctime()`?

time() return a variable of ytpe time_t, whereas ctime() return a pointer to a char.
time() can use a NULL pointer for input, whereas ctime() need to use a const pointer for input.

7. What is wrong with this code snippet?
```C
free(ptr);
free(ptr);
```
Use free() is telling the computer that you don't need that space anymore, 
so it marks that space as available for other data.
free twice will cause the dangling pointer problem.

8. What is wrong with this code snippet?
```C
free(ptr);
printf("%s\n", ptr);
```
free() is called on ptr so ptr is no long valid, but on the next line ptr is called by printf()
and this will cause a problem.

9. How can one avoid the previous two mistakes? 

after free the pointer, set the pointer to NULL.

### `struct`, `typedef`s, and a linked list
10. Create a `struct` that represents a `Person`. Then make a `typedef`, so that `struct Person` can be replaced with a single word. A person should contain the following information: their name (a string), their age (an integer), and a list of their friends (stored as a pointer to an array of pointers to `Person`s).
```C
struct Person {
	char* name;
	int age;
	struct Person** friends;
};
typedef struct Person person_t;
```

11. Now, make two persons on the heap, "Agent Smith" and "Sonny Moore", who are 128 and 256 years old respectively and are friends with each other.
```C
int main() {
	person_t* AgentSmith = (person_t*) malloc(sizeof(person_t));
	person_t* SonnyMoore = (person_t*) malloc(sizeof(person_t));
	AgentSmith->name = "Agent Smith";
	AgentSmith->age = 128;
	AgentSmith->friends = (person_t**) malloc(sizeof(person_t*));
	AgentSmith->friends[0] = SonnyMoore;
	SonnyMoore->name = "Sonny Moore";
	SonnyMoore->age = 256;
	SonnyMoore->friends = (person_t**) malloc(sizeof(person_t*));
	SonnyMoore->friends[0] = AgentSmith;
	return 0;
}
```

### Duplicating strings, memory allocation and deallocation of structures
Create functions to create and destroy a Person (Person's and their names should live on the heap).
12. `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).
```C
person_t* create(char* aName, int anAge) {
	person_t* p = (person_t*) malloc(sizeof(person_t));
	p->name = strdup(aName);
	p->age = anAge;
	p->friends = (person_t**) malloc(sizeof(person_t*) * 10);
	return p;
}
```
We initialize all fields to avoid some fields containing random values.

13. `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.
```C
void destroy(person_t* p) {
	free(p->name);
	free(p->friends);
	memset(p, 0, sizeof(person_t));
	free(p);
}
```

## Chapter 5 

Text input and output and parsing using `getchar`, `gets`, and `getline`.

### Reading characters, trouble with gets
1. What functions can be used for getting characters from `stdin` and writing them to `stdout`?

getchar(), putchar(int)

2. Name one issue with `gets()`.

When the size read by gets() is greater than the size of the input string, overflow will occur which will corropt the content of another variable

### Introducing `sscanf` and friends
3. Write code that parses the string "Hello 5 World" and initializes 3 variables to "Hello", 5, and "World".
```C
int main() {
	char* data = "Hello 5 World";
	char first[10];
	int second;
	char third[10];
	sscanf(data, "%9s %d %9s", first, &second, third);
	return 0;
}
```

### `getline` is useful
4. What does one need to define before including `getline()`?

a pointer to a buffer of type char and a capacity variable of type size_t

5. Write a C program to print out the content of a file line-by-line using `getline()`.
```C
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
void printFile(FILE* f) {
	if (!f) {
		exit(EXIT_FAILURE);
	}
	char* buffer = NULL;
	size_t capacity = 0;
	while (!feof(f)) {
		ssize_t result = getline(&buffer, &capacity, f);
		if (result == -1) exit(EXIT_FAILURE);
		printf("%s", buffer);
	}
	free(buffer);
}
```

## C Development

These are general tips for compiling and developing using a compiler and git. Some web searches will be useful here

1. What compiler flag is used to generate a debug build?

-g flag

2. You modify the Makefile to generate debug builds and type `make` again. Explain why this is insufficient to generate a new build.

need to use `make debug` instead of `make` to have extra flags like -DDEBUG and -g.

3. Are tabs or spaces used to indent the commands after the rule in a Makefile?

lines that begin with TAB are assumed to be part of a recipe, 
and lines that do not begin with TAB cannot be part of a recipe.
All other uses of indentation are optional and irrelevant.

4. What does `git commit` do? What's a `sha` in the context of git?

"SHA" stands for Simple Hashing Algorithm.
The checksum is the result of combining all the changes in the commit and feeding them to an algorithm that generates 40-character strings.

5. What does `git log` show you?

Shows the commit logs.

6. What does `git status` tell you and how would the contents of `.gitignore` change its output?

Show the working tree status;
the files that showed by `git status` will be ignored and not appear in the output

7. What does `git push` do? Why is it not just sufficient to commit with `git commit -m 'fixed all bugs' `?

Updates remote refs using local refs, while sending objects necessary to complete the given refs;

We don't know which bugs we have fixed.

8. What does a non-fast-forward error `git push` reject mean? What is the most common way of dealing with this?

This means that your subversion branch and your remote git master branch do not agree on something. 
Some change was pushed/committed to one that is not in the other;

Using `gitk --all`, and it will give you a clue about what went wrong.

## Optional (Just for fun)
- Convert your a song lyrics into System Programming and C code and share on Piazza.
- Find, in your opinion, the best and worst C code on the web and post the link to Piazza.
- Write a short C program with a deliberate subtle C bug and post it on Piazza to see if others can spot your bug.
- Do you have any cool/disastrous system programming bugs you've heard about? Feel free to share with your peers and the course staff on piazza.
