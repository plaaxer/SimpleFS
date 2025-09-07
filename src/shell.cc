#include "fs.h"
#include "disk.h"
#include "graphic_interface.cpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class File_Ops
{
public:
    static int do_copyin(const char *filename, int inumber, INE5412_FS *fs);

    static int do_copyout(int inumber, const char *filename, INE5412_FS *fs);

};

using namespace std;

int main( int argc, char *argv[] )
{

	if(argc != 3) {
		cout << "use: " << argv[0] << " <diskfile> <nblocks>\n";
		return 1;
	}


    Disk disk(argv[1], atoi(argv[2]));

    INE5412_FS fs(&disk);

	graphic_interface::SimpleFSInterface interface(&fs);

	cout << "opened emulated disk image " << argv[1] << " with " << disk.size() << " blocks\n";

	interface.run();

	cout << "closing emulated disk.\n";
	disk.close();

	return 0;
}
