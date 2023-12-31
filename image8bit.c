/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec: 113711 Name: Mariana Freitas Ribeiro 
// 
// 
// 
// Date: 22/11/2023
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
// 
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8* pixel; // pixel data (a raster scan)
};


// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
// 
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char* errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char* ImageErrMsg() { ///
  return errCause;
}


// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success = 
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
// 
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
// 
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
// 
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)


// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char* failmsg) {
  errCause = (char*)(condition ? "" : failmsg);
  return condition;
}


/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) { ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
  
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!


/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval) { ///
  assert (width >= 0);
  assert (height >= 0);
  assert (0 < maxval && maxval <= PixMax);
  Image img = (Image)malloc(sizeof(struct image)); //initialize the pointer.
  img->width = width; 
  img->height = height;
  img->maxval = maxval;
  img->pixel = (uint8*)calloc(width*height, sizeof(uint8)); //using calloc instead of malloc for every value in the memory that is allocated to be set to 0, creating the black image of the size height*width
  if (img == NULL || img->pixel ==  NULL){ 
    errCause = "Not enough memory - memory allocation failed";
    return NULL;
  }
  return img; //return img which is a pointer to acceptable values containing the width, height and maxval of this image
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image *imgp) { ///
  assert (imgp != NULL);
  Image img = *imgp;   //dereference the pointer;
  free(img->pixel);    //free the memory in the 1D array;
  free(img);           //free the rest of the memory;
  *imgp = NULL;        //delete the pointer;
}


/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char* filename) { ///
  int w, h;
  int maxval;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  int success = 
  check( (f = fopen(filename, "rb")) != NULL, "Open failed" ) &&
  // Parse PGM header
  check( fscanf(f, "P%c ", &c) == 1 && c == '5' , "Invalid file format" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &w) == 1 && w >= 0 , "Invalid width" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &h) == 1 && h >= 0 , "Invalid height" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax , "Invalid maxval" ) &&
  check( fscanf(f, "%c", &c) == 1 && isspace(c) , "Whitespace expected" ) &&
  // Allocate image
  (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
  // Read pixels
  check( fread(img->pixel, sizeof(uint8), w*h, f) == w*h , "Reading pixels" );
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL) fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char* filename) { ///
  assert (img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE* f = NULL;

  int success =
  check( (f = fopen(filename, "wb")) != NULL, "Open failed" ) &&
  check( fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed" ) &&
  check( fwrite(img->pixel, sizeof(uint8), w*h, f) == w*h, "Writing pixels failed" ); 
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (f != NULL) fclose(f);
  return success;
}


/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///
  assert (img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img) { ///
  assert (img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///
  assert (img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8* min, uint8* max) { ///
  assert (img != NULL);
  for ( int i = 0; i < ImageHeight(img)*ImageWidth(img) ; i++){
    if ( *min > img->pixel[i]){*min = img->pixel[i];}
    if ( *max < img->pixel[i]){*max = img->pixel[i];}
  }
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
  assert (img != NULL);
  return (0 <= x && x < ImageWidth(img)) && (0 <= y && y < ImageHeight(img));
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  if (w <= 0 || h <= 0) {return 0;}
  
  // making sure width and height values are not negative (which if they were to be, the image cropped could potentially give an inverted 
   // cropped image) and also check if the position (x,y) is inside img.
  return ( (ImageValidPos(img, x, y)) && ((x+w)<=ImageWidth(img)) && ((y+h)<=ImageHeight(img)) ); // returns 1 if the rectangle sides, starting from the point (x,y), are inside the image area.
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to 
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel. 
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {
  assert( (x >= 0) && (y >= 0) );
  int index = x + y*ImageWidth(img); //for every y, we add another line = width elements  
  assert (0 <= index && index < ImageWidth(img)*ImageHeight(img));
  return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (read)
  return img->pixel[G(img, x, y)];
} 

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
} 


/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.


/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) { ///
  assert (img != NULL);
  for (int x = 0; x < ImageWidth(img); x++){ //for every coordinate for x ∈ [0,width]
    for (int y = 0; y < ImageHeight(img); y++){ // & y ∈ [0,height],
      ImageSetPixel(img, x, y, abs(ImageMaxval(img)-ImageGetPixel(img, x, y))); // the pixel value ∈ [0,255] will be it's inverse, therefore 255 - absolute (value)
    }
  }
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) { ///
  assert (img != NULL);
  for (int x = 0; x < ImageWidth(img); x++){ //for every coordinate for x ∈ [0,width]
    for (int y = 0; y < ImageHeight(img); y++){ // & y ∈ [0,height],
      ImageSetPixel(img, x, y, ( (ImageGetPixel(img, x, y)>=thr) * ImageMaxval(img)) );// ImageGetPixel returns the level of each pixel. If said level is bigger or equals
      //to thr, it returns 1, if lower, returns 0. Then I multiply that result by the maxval: if true then it becomes 255 (maxvalue), if false it stays 0 (0*255 = 0).
    }
  }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor) { ///
  assert (img != NULL);
  assert( factor > 0 );
  for (int x = 0; x < ImageWidth(img); x++){ //for every coordinate for x ∈ [0,width]
    for (int y = 0; y < ImageHeight(img); y++){ // & y ∈ [0,height],
      uint8 new_brightness = (ImageGetPixel(img, x , y)*factor)+0.5; // the new brightness will be = to the value of the pixel * factor rounded up,
      if (new_brightness > 255){ //if the new brightness > 255, 
        new_brightness = ImageMaxval(img);// then it becomes = 255
        }
      ImageSetPixel(img, x , y, new_brightness); // and finally set the new brightness to the pixel
    }
  }
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
/// 
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint: 
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees anti-clockwise.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img) { ///
  assert (img != NULL);
  Image new_img = ImageCreate(ImageHeight(img), ImageWidth(img), ImageMaxval(img)); //create an image with the height = old width and width = old height size
  for (int x = 0; x < ImageWidth(img); x++){ //for every coordinate for x ∈ [0,width]
    for (int y = 0; y < ImageHeight(img); y++){ // & y ∈ [0,height],
      int width = ImageHeight(img)-y-1;
      int height = x;
      ImageSetPixel(new_img, x, y, ImageGetPixel(img, width, height));// The first line of pixels (with y = 0) will become the 
      // last column of pixels (with x = img->height) taking values ∈ [0,height], then the second line of pixels (y = 1) will 
      // become the second last column of pixels (x = old image height -1 ), etc... until the last line of pixels (with y = width) becomes
      // the first column of pixels (x = 0). We can also look at this from a mathematical prespective
      // and apply a change of variables of: ( x =-y ) & ( y = x )  
    }
  }
  if (new_img == NULL){
    errCause = "Not enough memory";
    return NULL;
  }
  return new_img;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img) { ///
  assert (img != NULL);
  Image new_img =  ImageCreate(ImageWidth(img), ImageHeight(img), ImageMaxval(img)); //create an image with exactly the same size and maxvalues
  for (int x = 0; x < ImageWidth(img); x++){ // for every coordinate for x ∈ [0,width]
    for (int y = 0; y < ImageHeight(img); y++){ // & y ∈ [0,height],
        int width = ImageWidth(img)-x-1; //the new width will start from the other end of the image, therefore, width-1 (for the first iteration)
        ImageSetPixel(new_img, x ,y,ImageGetPixel(img, width, y)); //set pixel for the value in the other end. 
    }
  }
  if (new_img == NULL){
    errCause = "Not enough memory";
    return NULL;
  }
  return new_img;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  assert (ImageValidRect(img, x, y, w, h));
  Image new_img = ImageCreate(w, h, ImageMaxval(img));
  for (int i = 0; i < w; i++){
    for (int j = 0; j < h; j++){
      int width = x+i;         
      int height = y+j;                               //starting at coords x,y
      uint8 level = ImageGetPixel(img, width, height);//get the pixels of the main image
      ImageSetPixel(new_img, i, j, level);            //and paste them in the new image
    }
  }
  if (new_img == NULL){
    errCause = "Not enough memory";
    return NULL;
  }
  return new_img;
}


/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, ImageWidth(img2), ImageHeight(img2))); //validate the space of img2 inside of img1, if there's not enough space, this will abort
  for (int i = 0; i < ImageWidth(img2); i++){
    for (int j = 0; j < ImageHeight(img2); j++){
      int width = i+x;              
      int height = j+y;       //starting at coords x,y
      ImageSetPixel(img1, width, height, ImageGetPixel(img2, i, j)); //get the img2 (small image) pixel starting from (0,0) and paste them into (the bigger image) img1
      //starting at coords x,y.  
    }
  }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));
  for (int i = 0; i < ImageWidth(img2); i++){
    for (int j = 0; j < ImageHeight(img2); j++){
      int width = i+x; // the new value for the pixel will be a mix of image 1 and image 2, and the portions will be decided by the alpha value which represents the percentage of img2 pixel to be added into img1 pixel
      int height = j+y; // it will have alpha*img2'pixel (which is the smaller image) and (the rest of the percentage of alpha)*img1'pixel. since c will
      int new_value = ImageGetPixel(img2, i, j)*alpha + ImageGetPixel(img1, width, height)*(1-alpha) + 0.5; // floor when changing to back to int, we add 0.5,
      if (new_value < 0) { // this way if the result decimal case is >= 0.5, it will get rounded up by 1.
        new_value = 0;}
      else if (new_value > ImageMaxval(img1) ){
        new_value = ImageMaxval(img1);}
      ImageSetPixel(img1, width, height, new_value); //finally set pixel to the new value;
    }
  }
}

/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) { //look for img2 inside img1 (img1 is bigger than img2)
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidPos(img1, x, y));             
  for (int i = 0; i < ImageWidth(img2); i++){     
    for (int j = 0; j < ImageHeight(img2); j++){  //given a certain x,y and img1, we're gonna compare img2 (the smaller image) with the sub-image of img1 starting
      int width = i+x;                            //at position x,y. the first pixel with unmatching values, makes the function return false for a similar sub-image
      int height = j+y;                           //while we start looking at pixels in img2 at (0,0), img1 starts at (x,y), then (x+1,j)... until (x+img2->width-1,y+img2->height-1)
      if (ImageGetPixel(img2, i, j) != ImageGetPixel(img1, width, height)){
        return 0;
      }
    }
  }
  return 1;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int* px, int* py, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  for (int i = 0; i < ImageWidth(img1)-ImageWidth(img2); i++){
    for (int j = 0; j < ImageHeight(img1)-ImageHeight(img2); j++){
      if (ImageGetPixel(img1, i, j) == ImageGetPixel(img2, 0, 0)){ //look for the value of the first pixel in img2 in img1. the first time this is true
        if (ImageMatchSubImage(img1, i, j, img2)){ //run the function to check if the image starting at position i,j matches. if true
          *px = i;  //change the pointer x coord value to i
          *py = j;  //y coord value to j
          return 1; //and finally return true
        }
      }
    }
  }
  return 0; //if the whole function runs and img2 isn't found in img1, return false
}


/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.
void ImageBlur(Image img, int dx, int dy) { ///
  assert (img != NULL);
  Image new_image = ImageCreate(ImageWidth(img), ImageHeight(img), ImageMaxval(img)); //we have to create a new temporary image to give the mean values, otherwise it
  if (new_image == NULL){errCause = "Not enough memory";} //won't be possible to correctly calculate them while we change them.
  for (int y = 0; y < ImageHeight(img); y++){
    for (int x = 0; x < ImageWidth(img); x++){
      int pixel_mean_sum = 0;                //this is where the rectangle would start. it will get negative coords until x = dx & y = dy,  
      int pixel_element_count = 0;           //so that we give the mean value always to the pixel centered in the middle.
      for (int i = -dy; i <= dy; i++){        //again, even tho these cords will be negative sometimes, we just need to ignore those invalid coords.
        for (int j = -dx; j <= dx; j++){      
          if (!(ImageValidPos(img, j+x, i+y))){continue;} //if the pixel isn't valid, just skip it. those pixels on the border of the image will have the mean
          //of less element count than those in the center on purpose
          pixel_mean_sum+=ImageGetPixel(img, j+x, i+y);   //sum all the pixel values inside the filter area
          pixel_element_count+=1; //sum the number of pixels inside the filter area
        }
      }
      double blur_pixel = ((double)(pixel_mean_sum) / pixel_element_count)+0.5; //do the math
      blur_pixel = (int)blur_pixel;
      ImageSetPixel(new_image, x, y, blur_pixel); //give the blurred effect to the pixel in the temporary image
    }
  }
  for (int y=0; y<ImageHeight(img); y++){
    for (int x=0; x<ImageWidth(img); x++){
      ImageSetPixel(img, x , y, ImageGetPixel(new_image, x, y)); //FINALLY, give the values from the temporary image to img
    }
  }
  ImageDestroy(&new_image); //and delete this temporary image
}

