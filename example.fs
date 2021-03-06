hsqs   )-|^             \      �	      �	      ��������D      �      �	      ��������all: none
clean:
	( cd magickeyboard ; make clean )
	( cd governor ; make clean )
jesus: clean
	tar -jcf - . | jesus src.rpiutils.tar.bz2
CFLAGS=-Wall
all: governor
governor: main.o
	gcc -o $@ $^
upload: clean
	scp * zerow:src/governor/
suinstall:
	chown root.root governor
	cp governor /usr/local/sbin
	chmod +s /usr/local/sbin/governor
clean:
	rm -f *.o governor	
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
FILE *ff;
char *choices[]={"ondemand","performance","powersave",NULL};
char **choice=NULL;

if (argc>1) {
	choice=choices;
	while (1) {
		if (!strcmp(argv[1],*choice)) break;
		choice++;
		if (!*choice) {
			fprintf(stderr,"Usage: governor [ondemand|performance|powersave]\n");
			return 0;
		}
	}
}

ff=fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor","r+b");
if (!ff) {
	fprintf(stderr,"Error opening file for writing\n");
	return -1;
}
if (choice) {
	if (0>fputs(*choice,ff)) fprintf(stderr,"Error writing value\n");
} else {
	char line[80];
	if (!fgets(line,80,ff)) return -1;
	fputs(line,stdout);
}
fclose(ff);
return 0;
}
CFLAGS=-Wall
all: magickeyboard-fn2
magickeyboard-fn2: main.o
	gcc -o $@ $^
suinstall:
	chown root.root magickeyboard-fn2
	cp magickeyboard-fn2 /usr/local/sbin
	chmod +s /usr/local/sbin/magickeyboard-fn2
clean:
	rm -f *.o magickeyboard-fn2	
#include <stdio.h>

int main(int argc, char **argv) {
FILE *ff;
ff=fopen("/sys/module/hid_apple/parameters/fnmode","wb");
if (!ff) {
	fprintf(stderr,"Error opening file for writing\n");
	return -1;
}
if (0>fputs("2",ff)) fprintf(stderr,"Error writing value\n");
fclose(ff);
return 0;
}
dwc_otg.lpm_enable=0 console=ttyAMA0,115200 console=tty1 root=/dev/mmcblk0p5 rw rootwait elevator=deadline ipv6.disable=1

options snd-usb-audio nrpacks=20
|� �    ¿z^   `   ����    �   �   �    �z^   �   ����    �   �   �    ��z^   �  ����    �  �  �    F�{^          -       �    ��y^   �  ����    �   �   �    ��y^   �  ����        �    ��y^          - *     �    ')[	   �  ����    {   {   �    )[
   #  ����    !   !   �    T�y^          < T     �    ÿz^          Q �    ۀ          $     MakefileH    main.c          �     Makefile�    main.c       	   �    
 cmdline.txt   snd_usb_audio.conf                Makefilel    governor�    magickeyboard<   noise��  �	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         