#include "balldetection.h"
#include "Vision/visionconstants.h"
#include "debug.h"
#include "debugverbosityvision.h"

void BallDetection::detectBall()
{
    VisionBlackboard* vbb = VisionBlackboard::getInstance();
    NUImage img = vbb->getOriginalImage();
    const LookUpTable& lut = vbb->getLUT();
    // BEGIN BALL DETECTION -----------------------------------------------------------------

    vector<Transition> transitions = vbb->getVerticalTransitions(VisionFieldObject::BALL);    

    #if VISION_FIELDOBJECT_VERBOSITY > 1
        debug << "BallDetection::detectBall() - number of ball transitions: " << transitions.size() << endl;
    #endif

    const GreenHorizon& green_horizon = vbb->getGreenHorizon();

    // Throw out points above the horizon
    vector<Transition>::iterator it;
    it = transitions.begin();
    while (it < transitions.end()) {
        if(green_horizon.isBelowHorizon(it->getLocation())) {
            it++;   //move to next transitions
        }
        else {
            #if VISION_FIELDOBJECT_VERBOSITY > 2
                debug << "BallDetection::detectBall() - transition thrown out, above GH: " << *it << endl;
            #endif
            it = transitions.erase(it);
        }
    }

    if (transitions.size() > 0) {

        // Arithmetic mean
        int x_mean = 0,
            y_mean = 0;
        for (unsigned int i = 0; i < transitions.size(); i++) {
            x_mean += transitions.at(i).getLocation().x;
            y_mean += transitions.at(i).getLocation().y;
        }
        x_mean /= transitions.size();
        y_mean /= transitions.size();

        // Standard deviation
        int x_dev = 0,
            y_dev = 0;
        for (unsigned int i = 0; i < transitions.size(); i++) {
            x_dev += abs(transitions.at(i).getLocation().x - x_mean);
            y_dev += abs(transitions.at(i).getLocation().y - y_mean);
        }
        x_dev /= transitions.size();
        y_dev /= transitions.size();

        //cout << transitions.size() << endl;


        // Statistical throw-out
        it = transitions.begin();
        while (it < transitions.end()) {
            if (abs(it->getLocation().x - x_mean) > x_dev || abs(it->getLocation().y - y_mean) > y_dev)
                it = transitions.erase(it);
            else
                it++;
        }

        // Geometric mean
        long double x_pos = 1,
               y_pos = 1;
        for (unsigned int i = 0; i < transitions.size(); i++) {
            int x = transitions.at(i).getLocation().x,
                y = transitions.at(i).getLocation().y;

            x_pos *= pow(x, 1.0/transitions.size());
            y_pos *= pow(y, 1.0/transitions.size());
        }
        //x_pos = pow(x_pos, 1.0/transitions.size());
        //y_pos = pow(y_pos, 1.0/transitions.size());        

        if (y_pos >= img.getHeight())
            y_pos = img.getHeight()-1;
        if (x_pos >= img.getWidth())
            x_pos = img.getWidth()-1;


        // Find ball centre (not occluded)        
        int top = y_pos,
            bottom = y_pos,
            left = x_pos,
            right = x_pos;
        int not_orange_count = 0;

        bool top_edge = false,
             bottom_edge = false,
             left_edge = false,
             right_edge = false;        

        // FIND BALL CENTRE (single iteration approach; doesn't deal great with occlusion)

        while (top >= 0 && not_orange_count <= VisionConstants::BALL_ORANGE_TOLERANCE) {
            if (ClassIndex::getColourFromIndex(lut.classifyPixel(img((int)x_pos, top))) != ClassIndex::orange) {
                not_orange_count++;
            }
            else {
                not_orange_count = 0;
            }
            top --;
        }
        top += not_orange_count;
        not_orange_count = 0;

        while (bottom < img.getHeight() && not_orange_count <= VisionConstants::BALL_ORANGE_TOLERANCE) {
            if (ClassIndex::getColourFromIndex(lut.classifyPixel(img((int)x_pos, bottom))) != ClassIndex::orange) {
                not_orange_count++;
            }
            else {
                not_orange_count = 0;
            }
            bottom ++;
        }
        bottom -= not_orange_count;
        not_orange_count = 0;

        while (left >= 0 && not_orange_count <= VisionConstants::BALL_ORANGE_TOLERANCE) {
            if (ClassIndex::getColourFromIndex(lut.classifyPixel(img(left, (int)y_pos))) != ClassIndex::orange) {
                not_orange_count++;
            }
            else {
                not_orange_count = 0;
            }
            left --;
        }
        left += not_orange_count;
        not_orange_count = 0;

        while (right < img.getWidth() && not_orange_count <= VisionConstants::BALL_ORANGE_TOLERANCE) {
            if (ClassIndex::getColourFromIndex(lut.classifyPixel(img(right, (int)y_pos))) != ClassIndex::orange) {
                not_orange_count++;
            }
            else {
                not_orange_count = 0;
            }
            right ++;
        }
        right -= not_orange_count;

        // CHECK IF POINT IS ON EDGE OF BALL (OR OCCLUDED)
        // OCCLUSION CHECK / COMPENSATION

        for (int i = left; i > left - VisionConstants::BALL_EDGE_THRESHOLD; i--) {
            if (i <= 0)
                break;
            else if (ClassIndex::getColourFromIndex(lut.classifyPixel(img(i, (int)y_pos))) == ClassIndex::green) {
                left_edge = true;
                break;
            }
        }
        for (int i = right; i < right + VisionConstants::BALL_EDGE_THRESHOLD; i++) {
            if (i >= img.getWidth()-1)
                break;
            else if (ClassIndex::getColourFromIndex(lut.classifyPixel(img(i, (int)y_pos))) == ClassIndex::green) {
                right_edge = true;
                break;
            }
        }
        for (int i = bottom; i < bottom + VisionConstants::BALL_EDGE_THRESHOLD; i++) {
            if (i >= img.getHeight()-1)
                break;
            else if (ClassIndex::getColourFromIndex(lut.classifyPixel(img((int)x_pos, i))) == ClassIndex::green) {
                bottom_edge = true;
                break;
            }
        }
        for (int i = top; i > top - VisionConstants::BALL_EDGE_THRESHOLD; i--) {
            if (i <= 0)
                break;
            else if (ClassIndex::getColourFromIndex(lut.classifyPixel(img((int)x_pos, i))) == ClassIndex::green) {
                top_edge = true;
                break;
            }
        }
        top_edge = true;

        PointType center;
        if (left_edge == true && right_edge == true && top_edge == true && bottom_edge == true) {
            center = PointType((right+left)/2,(top+bottom)/2);
        }
        else if (left_edge == true && right_edge == true) {
            if (top_edge == true && bottom_edge == false) {
                center = PointType((right+left)/2, min((top+(top+right-left))/2, img.getHeight()-1));                
            }
            else if (top_edge == false && bottom_edge == true) {
                center = PointType((right+left)/2, max((bottom+(bottom-right+left))/2, 0));
            }
            else {
                center = PointType((right+left)/2,(top+bottom)/2);
            }
        }
        else if (top_edge == true && bottom_edge == true) {
            if (left_edge == true && right_edge == false) {
                center = PointType(min((left+(left+bottom-top))/2, img.getWidth()-1),(top+bottom)/2);
            }
            else if (left_edge == false && right_edge == true) {
                center = PointType(max((right+(right-bottom+top))/2, 0),(top+bottom)/2);
            }
            else {
                center = PointType((right+left)/2,(top+bottom)/2);
            }
        }
        else {
            center = PointType((right+left)/2,(top+bottom)/2);
        }

        if (!(center.x ==1 and center.y==1) && bottom-top > 0 && right-left > 0) {

            // CHECK FOR PIXEL DENSITY
            int count = 0;
            for (int i = left; i < right; i++) {
                for (int j = top; j < bottom; j++) {
                    if (ClassIndex::getColourFromIndex(lut.classifyPixel(img(i, j))) == ClassIndex::orange)
                        count++;
                }
            }
            if (float(count)/((right-left)*(bottom-top)) >= VisionConstants::BALL_MIN_PERCENT_ORANGE) {
                Ball newball(center, max((right-left), (bottom-top))*0.5);
                vbb->addBall(newball);
            }
            else {
                #if VISION_FIELDOBJECT_VERBOSITY > 1
                    debug << "BallDetection::detectBall - ball thrown out on percentage contained orange" << endl;
                #endif
            }
        }
        else {
            #if VISION_FIELDOBJECT_VERBOSITY > 1
                debug << "BallDetection::detectBall - (1,1) ball thrown out" << endl;
            #endif
        }

    }
}
