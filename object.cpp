
/*--------------------------------------------------------------------

******************************************************************************
  * @file       object.cpp
  * @author     Claudio Vilao - ROBOFEI-HT - FEI
  * @version    V0.0.1
  * @created    24/04/2015
  * @Modified   18/02/2016
  * @e-mail
  * @brief      Semi-automatic Object selection tool - A pre-trained Haar 
		classifier will be executed at each image giving a most likely
		ball position.  
  ****************************************************************************

Para compilar: 
g++ -std=c++0x -I/usr/include/opencv object.cpp -o object -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann -Wno-write-string

Para executar
./object "arquivo de texto com as imagens" "Diretório das imagens"
./object Positive.txt Positive/

/--------------------------------------------------------------------*/




#include <opencv/cv.h>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>

// for filelisting
#include <stdio.h>
#include <sys/io.h>
// for fileoutput
#include <string>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/types.h>

using namespace std;

IplImage* image=0;
IplImage* image2=0;
CvRect *r=0;

CvHaarClassifierCascade *cascade;
CvMemStorage            *storage;

CvPoint* RecInit = new CvPoint();
CvPoint* RecEnd = new CvPoint();

void detect( IplImage *img );

int roi_x0=0;
int roi_y0=0;
int roi_x1=0;
int roi_y1=0;
int numOfRec=0;
int startDraw = 0;
bool finished = false;
char* window_name="<SPACE>add <B>save and load next <ESC>exit";

string IntToString(int num)
{
    ostringstream myStream; //creates an ostringstream object
    myStream << num << flush;
    return(myStream.str()); //returns the string form of the stringstream object
};

void on_mouse(int event,int x,int y,int flag, void *param)
{
    if(event==CV_EVENT_LBUTTONDOWN)
    {
        if(!startDraw)
        {
            roi_x0=x;
            roi_y0=y;
            startDraw = 1;
            finished = false;
        } else {
            roi_x1=x;
            roi_y1=y;
            startDraw = 0;
            finished = true;
        }
    }
    if(event==CV_EVENT_MOUSEMOVE)
    {
        image2=cvCloneImage(image);
        cvLine(image2, cvPoint(x, 0), cvPoint(x, 1080), CV_RGB(255,255,0),1);
        cvLine(image2, cvPoint(0, y), cvPoint(1920, y), CV_RGB(255,255,0),1);
        if (startDraw)
            cvRectangle(image2,cvPoint(roi_x0,roi_y0),cvPoint(x,y),CV_RGB(255,0,255),1);
        if (finished)
            cvRectangle(image2,cvPoint(roi_x0,roi_y0),cvPoint(roi_x1,roi_y1),CV_RGB(255,0,255),1);
        cvShowImage(window_name,image2);
        cvReleaseImage(&image2);
    }
}

int main(int argc, char** argv)
{
    char iKey=0;
    string strPrefix;
    string strPostfix;
    string input_directory;
    string output_file;
    int index = 0;

    if(argc != 3) {
        fprintf(stderr, "%s output_info.txt raw/data/directory/\n", argv[0]);
        return -1;
    }

    input_directory = argv[2];
    output_file = argv[1];


    char      *filename = "Ball.xml"; //Name of the classifier
    cascade = ( CvHaarClassifierCascade* )cvLoad( filename, 0, 0, 0 );
    storage = cvCreateMemStorage(0);


    /* Get a file listing of all files with in the input directory */
    DIR    *dir_p = opendir (input_directory.c_str());
    struct dirent *dir_entry_p;

    if(dir_p == NULL) {
        fprintf(stderr, "Failed to open directory %s\n", input_directory.c_str());
        return -1;
    }

    fprintf(stderr, "Object: Input Directory: %s  Output File: %s\n", input_directory.c_str(), output_file.c_str());

    //    init highgui
    cvAddSearchPath(input_directory);

    cvNamedWindow("<SPACE>add <B>save and load next <ESC>exit",1);

    cvSetMouseCallback("<SPACE>add <B>save and load next <ESC>exit",on_mouse, NULL);

    fprintf(stderr, "Opening directory...");
    //    init output of rectangles to the txt file
    ofstream output(output_file.c_str(), ios::app);
    fprintf(stderr, "done.\n");
//    fprintf(stderr, output_file.c_str());
    
    int ended = 0;
    ifstream test("last.txt");
    if (test.is_open())
    {
        string line;
        getline(test, line);
        ended = stoi (line);
        //cout << line;
    }
    test.close();
    ofstream last("last.txt");

    while((dir_entry_p = readdir(dir_p)) != NULL)
    {
        if(index >= ended){
            numOfRec=0;

            if(strcmp(dir_entry_p->d_name, ""))
                fprintf(stderr, "Examining file %s\n", dir_entry_p->d_name);

            /* TODO: Assign postfix/prefix info */
            strPostfix="";


            strPrefix=input_directory+dir_entry_p->d_name;
            //strPrefix+=bmp_file.name;
            fprintf(stderr, "Loading image %s\n", strPrefix.c_str());

            if((image=cvLoadImage(strPrefix.c_str(),1)) != 0)
            {

                //   Current image
                do
                {
                    detect(image);
                    roi_x0=RecInit->x;
                    roi_y0=RecInit->y;
                    roi_x1=RecEnd->x;
                    roi_y1=RecEnd->y;
                    cvShowImage("<SPACE>add <B>save and load next <ESC>exit",image);

                    // used cvWaitKey returns:
                    //    <B>=66        save added rectangles and show next image
                    //    <b>=98
                    //    <ESC>=27        exit program
                    //    <Space>=32        add rectangle to current image
                    //  any other key clears rectangle drawing only
                    iKey=cvWaitKey(0);
                    switch(iKey)
                    {
                        case 27:
                            last << index;
                            cvReleaseImage(&image);
                            cvDestroyWindow("<SPACE>add <B>save and load next <ESC>exit");
                            return 0;
                            
                        case 32:
                            numOfRec++;
                            printf("   %d. rect x=%d\ty=%d\tx2h=%d\ty2=%d\n",numOfRec,roi_x0,roi_y0,roi_x1,roi_y1);
                            //printf("   %d. rect x=%d\ty=%d\twidth=%d\theight=%d\n",numOfRec,roi_x1,roi_y1,roi_x0-roi_x1,roi_y0-roi_y1);
                            // currently two draw directions possible:
                            //        from top left to bottom right or vice versa
                            if(roi_x0<roi_x1 && roi_y0<roi_y1)
                            {
                                printf("   %d. rect x=%d\ty=%d\twidth=%d\theight=%d\n",numOfRec,roi_x0,roi_y0,roi_x1-roi_x0,roi_y1-roi_y0);
                                // append rectangle coord to previous line content
                                strPostfix+=" "+IntToString(roi_x0)+
                                            " "+IntToString(roi_y0)+
                                            " "+IntToString(roi_x1-roi_x0)+
                                            " "+IntToString(roi_y1-roi_y0);
                            } else { //(roi_x0>roi_x1 && roi_y0>roi_y1)
                                printf("   %d. rect x=%d\ty=%d\twidth=%d\theight=%d\n",numOfRec,roi_x1,roi_y1,roi_x0-roi_x1,roi_y0-roi_y1);
                                // append rectangle coord to previous line content
                                strPostfix+=" "+IntToString(roi_x1)+
                                            " "+IntToString(roi_y1)+
                                            " "+IntToString(roi_x0-roi_x1)+
                                            " "+IntToString(roi_y0-roi_y1);
                            }

                            break;
                    }
                }
                while(iKey!=98);
                {
                // save to file  <rel_path>\bmp_file.name numOfRec x0 y0 width0 height0 x1 y1 width1 height1...
                // Se houverem mais objetos x2 y2 largura2 altura2 x3 y3 largura3 altura3
                    if(numOfRec>0 && iKey==98)
                    {
                        output << strPrefix << " "<< numOfRec << strPostfix <<"\n";
                        cvReleaseImage(&image);
                    } else {
                        fprintf(stderr, "Failed to load image, %s\n", strPrefix.c_str());
                    }
                }
            }   
        }
        index++;
    }
    
    last << index;
    last.close();
    output.close();
    cvDestroyWindow("<SPACE>add <B>save and load next <ESC>exit");
    closedir(dir_p);

    return 0;
}


void detect(IplImage *img)
{
    int i;

    CvSeq *object = cvHaarDetectObjects(
            img,
            cascade,
            storage,
            1.29, //-------------------SCALE FACTOR 1,5
            5,//------------------MIN NEIGHBOURS 2 7            
            1,//---------------------- 1
                      // CV_HAAR_DO_CANNY_PRUNING,
            cvSize(30,30), // ------MINSIZE
            cvSize(1920,1080) );//---------MAXSIZE

    for( i = 0 ; i < ( object ? object->total : 0 ) ; i++ )
        {
            CvRect *r = ( CvRect* )cvGetSeqElem( object, i );

		        RecInit->x = r->x;
			RecInit->y = r->y;
		
			RecEnd->x = r->x + r->width;
			RecEnd->y = r->y + r->height;

            cvRectangle( img, cvPoint(r->x,r->y), cvPoint(r->x + r->width,r->y + r->height), CV_RGB( 64, 255, 64 ), 3, 8, 0 );
		

        }
		
}

