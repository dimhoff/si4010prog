LDFLAGS=-lusb
C2_BUS_DRV=c2_hw_fx2

all: si4010prog ec2_emul

si4010prog: main.o si4010.o $(C2_BUS_DRV).o dehexify.o
	$(CC) $(CFLAGS) -o si4010prog $^ $(LDFLAGS)

ec2_emul: ec2_emul.o si4010.o $(C2_BUS_DRV).o ec2_cksum.o
	$(CC) $(CFLAGS) -o ec2_emul $^ $(LDFLAGS)

clean:
	rm -fr si4010prog main.o si4010.o $(C2_BUS_DRV).o dehexify.o
	rm -fr ec2_emul ec2_emul.o ec2_cksum.o
