#include "qrReader.h"

void qrReader::drawFinders(Mat &img) 
{
    if(possibleCenters.size()==0) {
        return;
    }

    for(int i=0;i<possibleCenters.size();i++) {
        Point2f pt = possibleCenters[i];
        float diff = estimatedModuleSize[i]*3.5f;

        Point2f pt1(pt.x-diff, pt.y-diff);
        Point2f pt2(pt.x+diff, pt.y+diff);
        rectangle(img, pt1, pt2, CV_RGB(255, 0, 0), 1);
    }
}

bool qrReader::find(const Mat& img)
{
    possibleCenters.clear();
    estimatedModuleSize.clear();

    int skipRows = 3;

    int stateCount[5] = {0};
    int currentState = 0;

    for(int row=skipRows-1; row<img.rows; row+=skipRows) {
		stateCount[0] = 0;
		stateCount[1] = 0;
		stateCount[2] = 0;
		stateCount[3] = 0;
		stateCount[4] = 0;
		currentState = 0;

		const uchar* ptr = img.ptr<uchar>(row);
		for(int col=0; col<img.cols; col++) {
			if(ptr[col]<128) {
				if((currentState & 0x1) == 1) {
					currentState++;
				}
					// Works for boths W->B and B->B
				stateCount[currentState]++;
			} else {
				if((currentState & 0x1)==1) {
					stateCount[currentState]++;
				} else {
					if (currentState == 4) {
						if(checkRatio(stateCount)) {
						//TODO do some more checks
						} else {
							currentState = 3;
							stateCount[0] = stateCount[2];
							stateCount[1] = stateCount[3];
							stateCount[2] = stateCount[4];
							stateCount[3] = 1;
							stateCount[4] = 0;
							continue;
						}
						currentState = 0;
						stateCount[0] = 0;
						stateCount[1] = 0;
						stateCount[2] = 0;
						stateCount[3] = 0;
						stateCount[4] = 0;
					 } else {
						 currentState++;
						 stateCount[currentState]++;
						}
					}
				}
		}
	}
    return (possibleCenters.size() >0);
}

bool qrReader::checkRatio(int stateCount[])
{
    int totalFinderSize = 0;
    for(int i=0; i<5; i++)
    {
        int count = stateCount[i];
        totalFinderSize += count;
        if(count==0)
            return false;
    }

    if(totalFinderSize<7)
        return false;

    // Calculate the size of one module
    int moduleSize = ceil(totalFinderSize / 7.0);
    int maxVariance = moduleSize/2;

    bool retVal= ((abs(moduleSize - (stateCount[0])) < maxVariance) &&
        (abs(moduleSize - (stateCount[1])) < maxVariance) &&
        (abs(3*moduleSize - (stateCount[2])) < 3*maxVariance) &&
        (abs(moduleSize - (stateCount[3])) < maxVariance) &&
        (abs(moduleSize - (stateCount[4])) < maxVariance));

    return retVal;
}
