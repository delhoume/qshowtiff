#include <TiffViewer.h>

int main(int argc, char** argv) {
   TiffViewer tiffviewer(800, 600);
   if (argc > 1) {
    tiffviewer.load(argv[1]);
   }
   return tiffviewer.run();
}