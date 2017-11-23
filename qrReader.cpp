#include "qrReader.h"

#define nan std::numeric_limits<float>::quiet_NaN();

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
							bool confirmed = handlePossibleCenter(img, stateCount, row, col);
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

bool qrReader::handlePossibleCenter(const Mat& img, int *stateCount, int row, int col) 
{
	int stateCountTotal = 0;
	for(int i=0;i<5;i++) {
		stateCountTotal += stateCount[i];
    }

    float centerCol = centerFromEnd(stateCount, col);
    float centerRow = crossCheckVertical(img, row, (int)centerCol, stateCount[2], stateCountTotal);
    if(isnan(centerRow)) {
    	return false;
    }
    centerCol = crossCheckHorizontal(img, centerRow, centerCol, stateCount[2], stateCountTotal);
    if(isnan(centerCol)) {
    	return false;
    }
    bool validPattern = crossCheckDiagonal(img, centerRow, centerCol, stateCount[2], stateCountTotal);
    if(!validPattern) {
    	return false;
    }
	Point2f ptNew(centerCol, centerRow);
    float newEstimatedModuleSize = stateCountTotal / 7.0f;
    bool found = false;
    int idx = 0;

	for(Point2f pt: possibleCenters) {
		Point2f diff = pt - ptNew;
		float dist = (float) sqrt(diff.dot(diff));

		if(dist < 10) {
			pt = pt + ptNew;
			pt.x /= 2.0f;
			pt.y /= 2.0f;
			estimatedModuleSize[idx] = (estimatedModuleSize[idx] + newEstimatedModuleSize)/2.0f;
			found - true;
			break;
		}
		idx++;
	}

	if(!found) {
		possibleCenters.push_back(ptNew);
		estimatedModuleSize.push_back(newEstimatedModuleSize);
	}
	return false;
}

float qrReader::crossCheckVertical(const Mat& img, int startRow, int centerCol, int centralCount, int stateCountTotal) 
{
	int maxRows = img.rows;
	int crossCheckStateCount[5] = {0};
	int row = startRow;
	while(row>=0 && img.at<uchar>(row, centerCol)<128) {
		crossCheckStateCount[2]++;
		row--;
	}
	if(row<0) {
		return nan;
	}
	while(row>=0 && img.at<uchar>(row, centerCol)>=128 && crossCheckStateCount[1]<centralCount) {
		crossCheckStateCount[1]++;
		row--;
	}

	if(row<0 || crossCheckStateCount[1]>=centralCount) {
		return nan;
	}

	while(row>=0 && img.at<uchar>(row, centerCol)<128 && crossCheckStateCount[0]<centralCount) {
		crossCheckStateCount[0]++;
		row--;
	}
	if(row<0 || crossCheckStateCount[0]>=centralCount) {
		return nan;
	}

	row = startRow+1;
	while(row<maxRows && img.at<uchar>(row, centerCol)<128) {
		crossCheckStateCount[2]++;
		row++;
	}
	if(row==maxRows) {
		return nan;
	}
	while(row<maxRows && img.at<uchar>(row, centerCol)>=128 && crossCheckStateCount[3]<centralCount) {
		crossCheckStateCount[3]++;
		row++;
	}
	if(row==maxRows || crossCheckStateCount[3]>=stateCountTotal) {
		return nan;
	}

	while(row<maxRows && img.at<uchar>(row, centerCol)<128 && crossCheckStateCount[4]<centralCount) {
		crossCheckStateCount[4]++;
		row++;
	}
	if(row==maxRows || crossCheckStateCount[4]>=centralCount) {
		return nan;
	}

	int crossCheckStateCountTotal = 0;
    for(int i=0;i<5;i++) {
		crossCheckStateCountTotal += crossCheckStateCount[i];
	}

	if(5*abs(crossCheckStateCountTotal-stateCountTotal) >= 2*stateCountTotal) {
		return nan;
	}

	float center = centerFromEnd(crossCheckStateCount, row);
	return checkRatio(crossCheckStateCount)?center:nan;
}

float qrReader::crossCheckHorizontal(const Mat& img, int centerRow, int startCol, int centerCount, int stateCountTotal) {
	int maxCols = img.cols;
	int stateCount[5] = {0};

	int col = startCol;
	const uchar* ptr = img.ptr<uchar>(centerRow);
	while(col>=0 && ptr[col]<128) {
		stateCount[2]++;
		col--;
	}
	if(col<0) {
		return nan;
	}

	while(col>=0 && ptr[col]>=128 && stateCount[1]<centerCount) {
		stateCount[1]++;
		col--;
	}
	if(col<0 || stateCount[1]==centerCount) {
		return nan;
	}

	while(col>=0 && ptr[col]<128 && stateCount[0]<centerCount) {
		stateCount[0]++;
		col--;
	}
	if(col<0 || stateCount[0]==centerCount) {
		return nan;
	}

	col = startCol + 1;
	while(col<maxCols && ptr[col]<128) {
	stateCount[2]++;
	col++;
	}
	if(col==maxCols) {
		return nan;
	}

	while(col<maxCols && ptr[col]>=128 && stateCount[3]<centerCount) {
		stateCount[3]++;
		col++;
	}
	if(col==maxCols || stateCount[3]==centerCount) {
		return nan;
	}

	while(col<maxCols && ptr[col]<128 && stateCount[4]<centerCount) {
		stateCount[4]++;
		col++;
	}
	if(col==maxCols || stateCount[4]==centerCount) {
		return nan;
	}
	int newStateCountTotal = 0;
	for(int i=0;i<5;i++) {
		newStateCountTotal += stateCount[i];
	}

	if(5*abs(stateCountTotal-newStateCountTotal) >= stateCountTotal) {
		return nan;
	}

	return checkRatio(stateCount)?centerFromEnd(stateCount, col):nan;
}

bool qrReader::crossCheckDiagonal(const Mat &img, float centerRow, float centerCol, int maxCount, int stateCountTotal) {
	int stateCount[5] = {0};

	int i=0;
	while(centerRow>=i && centerCol>=i && img.at<uchar>(centerRow-i, centerCol-i)<128) {
		stateCount[2]++;
		i++;
	}
	if(centerRow<i || centerCol<i) {
		return false;
	}

	while(centerRow>=i && centerCol>=i && img.at<uchar>(centerRow-i, centerCol-i)>=128 && stateCount[1]<=maxCount) {
		stateCount[1]++;
		i++;
                                                }
	if(centerRow<i || centerCol<i || stateCount[1]>maxCount) {
		return false;
	}

	while(centerRow>=i && centerCol>=i && img.at<uchar>(centerRow-i, centerCol-i)<128 && stateCount[0]<=maxCount) {
		stateCount[0]++;
		i++;
	}
	if(stateCount[0]>maxCount) {
		return false;
	}

	int maxCols = img.cols;
	int maxRows = img.rows;
	i=1;
	while((centerRow+i)<maxRows && (centerCol+i)<maxCols && img.at<uchar>(centerRow+i, centerCol+i)<128) {
		stateCount[2]++;
		i++;
	}
	if((centerRow+i)>=maxRows || (centerCol+i)>=maxCols) {
		return false;
	}

	while((centerRow+i)<maxRows && (centerCol+i)<maxCols && img.at<uchar>(centerRow+i, centerCol+i)>=128 && stateCount[3]<maxCount) {
		stateCount[3]++;
		i++;
	}
	if((centerRow+i)>=maxRows || (centerCol+i)>=maxCols || stateCount[3]>maxCount) {
		return false;
	}

	while((centerRow+i)<maxRows && (centerCol+i)<maxCols && img.at<uchar>(centerRow+i, centerCol+i)<128 && stateCount[4]<maxCount) {
		stateCount[4]++;
		i++;
	}
	if((centerRow+i)>=maxRows || (centerCol+i)>=maxCols || stateCount[4]>maxCount) {
		return false;
	}

	int newStateCountTotal = 0;
	for(int j=0;j<5;j++) {
		newStateCountTotal += stateCount[j];
	}

	return (abs(stateCountTotal - newStateCountTotal) < 2*stateCountTotal) && checkRatio(stateCount);
}
