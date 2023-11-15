include ../../tela.mak

# Test executables and subdirectories
TESTS := dev_null day monday.sh time.sh

# Recipes for building compiled test executables
dev_null: dev_null.c
day: day.c $(TELA_OBJ)

clean:
	rm -f dev_null day
