1

조교님 필독!!

메일 드렸던 것처럼 PA3 컴파일 하실 때 옵션을

```
.cc.o:
    ${CC} ${CFLAGS} -c -std=c++11 $<
```

로 주셔야 컴파일이 됩니다!!!

컴파일 안된다고 0점 주지 말아주세요..ㅠㅠㅠㅠ

std::function을 지원을 안해서 그런건데, 이걸 지금와서 다 바꿀수가 없으니..ㅠㅠ

아래는 makefile 전체입니다!!


```
SUPPORTDIR= ../cool-support
LIB= 
SRC= semant.cc semant.h cool-tree.h cool-tree.handcode.h 
CSRC= semant-phase.cc symtab_example.cc  handle_flags.cc  ast-lex.cc ast-parse.cc utilities.cc stringtab.cc dumptype.cc tree.cc cool-tree.cc
CFLAGS= -g -Wall -Wno-unused -Wno-deprecated -DDEBUG ${CPPINCLUDE}
CPPINCLUDE= -I. -I${SUPPORTDIR}/include 
CC= g++
CFIL= semant.cc ${CSRC} 
OBJS= ${CFIL:.cc=.o}


all: semant

.cc.o:
	${CC} ${CFLAGS} -c -std=c++11 $<

SEMANT_OBJS := ${filter-out symtab_example.o,${OBJS}}

semant:  ${SEMANT_OBJS} 
	${CC} ${CFLAGS} ${SEMANT_OBJS} ${LIB} -o semant

symtab_example: symtab_example.cc 
	${CC} ${CFLAGS} symtab_example.cc ${LIB} -o symtab_example

${CSRC}:
	-ln -s $(SUPPORTDIR)/src/$@ $@


clean :
	-rm -f core ${SEMANT_OBJS} semant *~ *.output

realclean: clean
	-rm -f ${CSRC} 
```