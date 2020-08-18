CC		=  gcc
CFLAGS	        += -std=c99 -Wall -g

INCDIR		= ./include
LIBDIR          = ./lib
SRCDIR          = ./src
BINDIR          = ./bin
CONF        	= ./conf
LOGS		= ./logs
INCLUDES        = -I $(INCDIR)
TARGET		= $(BINDIR)/prog

.PHONY: clean test

# pattern da .c ad eseguibile 
$(TARGET): ./prog.o ./casse.o ./clienti.o
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lpthread


# pattern per la generazione di un .o da un .c nella directory SRCDIR
prog.o: $(SRCDIR)/prog.c $(INCDIR)/utils.h 
	$(CC) $(CFLAGS) $(INCLUDES) -c $<
 

casse.o: $(SRCDIR)/casse.c $(INCDIR)/utils.h 
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  


clienti.o: $(SRCDIR)/clienti.c $(INCDIR)/utils.h 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< 


# per eseguire il test1
test: $(TARGET)
	@echo "running test1"
	$(CONF)/signalint.sh & $(TARGET)
	$(CONF)/analisi.sh
	@echo "test1 OK"

clean		: 
	-rm -f $(TARGET)

cleanall	: clean
	-find . \( -name "*~" -o -name "*.o" \) -exec rm -f {} \;
	-rm -f  $(LOGS)/log.txt



