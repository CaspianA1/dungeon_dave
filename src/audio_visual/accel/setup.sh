
# echo "Good day! Here's a quote:"
# fortune
ssh-add ~/.ssh/id_ed25519

prog() {
	cd ~/programming/$1
}

down() {
	git fetch
	git pull
}

update() {
	printf "Enter a commit: "
	read msg
	git add .
	git commit -m "$msg"
}

up() {
	update
	git push
}

search() {
	dir=$1
	if [ "$dir" = "" ]; then
		echo "Please enter a directory."
	else
		regex=${@:2}
		grep -rni "$regex" $dir/*
	fi
}

init_c() {
	mkdir $1
	cd $1
	mkdir src bin other
	touch Makefile src/$1.c src/$1.h other/scrap_code.txt

	echo -e "CC = gcc
DEBUGGER = gdb

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address # This is for a debug build, not a debugger session
BUILD_TYPE = \$(OPTIMIZE)

CFLAGS = -Wall -Wformat -Wpedantic -Wextra
LDFLAGS = -o bin/\$(OUT) src/\$(OUT).c

OUT = $1

all: build run

run:
	./bin/\$(OUT)

build:
	\$(CC) \$(CFLAGS) \$(BUILD_TYPE) \$(LDFLAGS)

asm:
	\$(CC) \$(CFLAGS) -S -masm=intel -o bin/\$(OUT).asm src/\$(OUT).c

debug:
	\$(CC) \$(CFLAGS) -ggdb3 \$(LDFLAGS)
	\$(DEBUGGER) bin/\$(OUT)

clean:
	rm -r bin/*" > Makefile

echo -e "#include \"$1.h\"

int main(void) {

}" > src/$1.c

	echo "Project '$1' was initialized successfully."
}

alias :q=exit
alias cl="clear && printf '\e[3J'"
alias rmd="rm -r"
alias reload="source ~/.bashrc"

alias l=ls
alias s=ls
alias sl=ls
alias la=ls
alias ake=make
alias mke=make

alias r="cl; make"
alias scheme="chez --script"
alias lisp="sbcl --load"

GIT_EDITOR="vim"
VISUAL="less"
EDITOR="vim"
