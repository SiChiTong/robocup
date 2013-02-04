#include "goaldetectorransac.h"
#include "Vision/visionblackboard.h"
#include "Vision/visionconstants.h"
#include "Vision/GenericAlgorithms/ransac.h"
#include "Vision/VisionTypes/visionline.h"
#include "Tools/Math/General.h"

#include <limits>
#include <stdlib.h>
#include <boost/foreach.hpp>

GoalDetectorRANSAC::GoalDetectorRANSAC()
{
    m_n = 10;               //min pts to line essentially
    m_k = 40;               //number of iterations per fitting attempt
    m_e = 8.0;              //consensus margin
    m_max_iterations = 3;  //hard limit on number of fitting attempts
}

vector<Goal> GoalDetectorRANSAC::run()
{
    VisionBlackboard* vbb = VisionBlackboard::getInstance();
    //get transitions associated with goals
    vector<ColourSegment> h_segments = vbb->getHorizontalTransitions(GOAL_COLOUR),
                          v_segments = vbb->getVerticalTransitions(GOAL_COLOUR);
    vector<Quad> candidates;
    vector<Goal> posts;

    //finds the edge lines and constructs goals from that
    vector<Point> start_points, end_points;
    vector<pair<VisionLine, vector<Point> > > ransac_results;
    vector<LSFittedLine> start_lines, end_lines;



    //get edge points
    BOOST_FOREACH(ColourSegment s, h_segments) {
        start_points.push_back(s.getStart());
        end_points.push_back(s.getEnd());
    }

    //use generic ransac implementation to find lines
    ransac_results = RANSAC::findMultipleModels<VisionLine, Point>(start_points, m_e, m_n, m_k, m_max_iterations);
    for(unsigned int i=0; i<ransac_results.size(); i++) {
        start_lines.push_back(LSFittedLine(ransac_results.at(i).second));
    }

    ransac_results = RANSAC::findMultipleModels<VisionLine, Point>(end_points, m_e, m_n, m_k, m_max_iterations);
    for(unsigned int i=0; i<ransac_results.size(); i++) {
        end_lines.push_back(LSFittedLine(ransac_results.at(i).second));
    }

    DataWrapper::getInstance()->debugPublish(DBID_GOAL_LINES_START, start_lines);
    DataWrapper::getInstance()->debugPublish(DBID_GOAL_LINES_END, end_lines);

    //Build candidates out of lines
    candidates = buildQuadsFromLines(start_lines, end_lines, VisionConstants::GOAL_RANSAC_MATCHING_TOLERANCE);

    cout << "candidates: " << candidates.size() << endl;

    //validity check
    removeInvalidPosts(candidates);
    cout << "after invalid: " << candidates.size() << endl;

    //overlap check
    overlapCheck(candidates);
    cout << "after overlap: " << candidates.size() << endl;

    posts = assignGoals(candidates);

    //Improves bottom centre estimate using vertical transitions
    BOOST_FOREACH(ColourSegment v, v_segments) {
        BOOST_FOREACH(Goal g, posts) {
            if(v.getEnd().y > g.getLocationPixels().y)
                g.setBase(v.getEnd());
        }
    }

    return posts;
}

//vector<Quad> GoalDetectorRANSAC::buildQuadsFromLines(const vector<LSFittedLine> &start_lines, const vector<LSFittedLine> &end_lines)
//{
//    vector<LSFittedLine>::const_iterator s_it = start_lines.begin(),
//                                         e_it = end_lines.begin(),
//                                         e_temp;

//    while(s_it != start_lines.end() && e_it != end_lines.end()) {
//        //move through until end line is after start line
//        //get the end points of each line
//        Point sp1 = s_it->getLeftPoint(),
//              sp2 = s_it->getRightPoint(),
//              ep1 = e_it->getLeftPoint(),
//              ep2 = e_it->getRightPoint(),
//              intersection;

//        //sort the points
//        double d1 = 0.5*( (sp1-ep1).abs() + (sp2-ep2).abs() ),
//               d2 = 0.5*( (sp2-ep1).abs() + (sp1-ep2).abs() );

//        if(d1 > d2) {
//            //sp1 should be paired with ep2
//            Point c(ep1); ep1=ep2; ep2=c;   //swap ep1 and ep2
//        }

//        //check if terminal points of start are left of terminal points of end
//        //if(sp1.x < ep1.x && sp1.x < ep2.x && sp2.x < ep1.x && sp2.x < ep2.x) {

//        if(sp1.x < ep1.x && sp2.x < ep2.x) {
//            if(s_it->getIntersection(*e_it, intersection)) {
//                //check if intersection point is below end points, otherwise don't consider
//                if(intersection.y > sp1.y && intersection.y > ep1.y &&
//                   intersection.y > sp2.y && intersection.y > ep2.y) {
//                    //consider

//                }
//                else {
//                    e_it++;
//                }
//            }
//            else {
//                //perfectly parallel lines - consider
//            }
//        }
//        else {
//            e_it++;
//        }





//        s_it++;
//    }

//    return vector<Quad>();
//}

vector<Quad> GoalDetectorRANSAC::buildQuadsFromLines(const vector<LSFittedLine>& start_lines, const vector<LSFittedLine>& end_lines, double tolerance)
{
    // (must match exactly) 0 <= tolerance <= 1 (any pair will be accepted)
    //
    // LSFittedLine objects contain lists of points and can be quite large,
    // therefore it is more efficient to pass by const reference and maintain
    // a list of matched end lines than to pass by copy so that the end lines
    // list can be shrunk.

    if( tolerance < 0 or tolerance > 1)
        throw "GoalDetectorRANSAC::buildQuadsFromLines - tolerance must be in [0, 1]";

    vector<Quad> quads;
    vector<bool> used(end_lines.size(), false);

    BOOST_FOREACH(const LSFittedLine& s, start_lines) {
        vector<bool> tried(used);   //consider all used lines tried
        vector<LSFittedLine>::const_iterator e_it;

        //try end lines in order of closeness
        for(unsigned int i = getClosestUntriedLine(s, end_lines, tried);
            i < end_lines.size();
            i = getClosestUntriedLine(s, end_lines, tried)) {

            const LSFittedLine& e = end_lines.at(i);

            //check angles
            if(s.getAngleBetween(e) < tolerance*mathGeneral::PI*0.5) { //dodgy way (linear with angle between)
            //if(min(a1/a2, a2/a1) <= (1-tolerance)) {
                //get the end points of each line
                Vector2<Point> sp = s.getEndPoints(),
                               ep = e.getEndPoints();
                //find lengths of line segments
                double l1 = (sp.x - sp.y).abs(),
                       l2 = (ep.x - ep.y).abs();
                //check lengths
                if(min(l1/l2, l2/l1) >= (1-tolerance)) {
                    //get num points
                    double n1 = s.getNumPoints(),
                           n2 = e.getNumPoints();
                    if(min(n1/n2, n2/n1) >= (1-tolerance)) {
                        //check start is to left of end
                        if(0.5*(sp[0].x + sp[1].x) < 0.5*(ep[0].x + ep[1].x)) {
                            //success
                            //order points
                            if(sp[0].y < sp[1].y) {
                                sp.transpose();
                            }
                            if(ep[0].y < ep[1].y) {
                                ep.transpose();
                            }
                            quads.push_back(Quad(sp.x, sp.y, ep.y, ep.x));  //generate candidate
                            used.at(i) = true;  //remove end line from consideration
                            cout << "success " << sp << ep << endl;
                            break;  //do not consider any more lines
                        }
                        else
                            cout << "line ordering fail: " << 0.5*(sp[0].x + sp[1].x) << " " << 0.5*(ep[0].x + ep[1].x) << endl;
                    }
                    else
                        cout << "num points fail: " << n1 << " " << n2 << endl;
                }
                else
                    cout << "length fail: " << l1 << " " << l2 << endl;
            }
            else
                cout << "angle fail: " << s.getAngleBetween(e) << " > " << tolerance*mathGeneral::PI*0.5 << endl;
        }
    }

    return quads;
}

unsigned int GoalDetectorRANSAC::getClosestUntriedLine(const LSFittedLine& start, const vector<LSFittedLine>& end_lines, vector<bool>& tried)
{
    if(end_lines.size() != tried.size())
        throw "GoalDetectorRANSAC::getClosestUntriedLine - 'end_lines' must match 'tried' in size";

    unsigned int best = end_lines.size();   //for if all have been tried
    double d_best = std::numeric_limits<double>::max();

    for(unsigned int i = 0; i < end_lines.size(); i++) {
        //check if tried yet
        if(!tried[i]) {
            //check if distance is smallest
            double d = start.averageDistanceBetween(end_lines[i]);
            if(d < d_best) {
                best = i;
                d_best = d;
            }
        }
    }

    if(best < end_lines.size())
        tried[best] = true; //mark as used

    return best;
}

vector<Point> GoalDetectorRANSAC::getEdgePointsFromSegments(const vector<ColourSegment> &segments)
{
    vector<Point> points;
    BOOST_FOREACH(ColourSegment s, segments) {
        points.push_back(Point(s.getStart().x, s.getStart().y));
        points.push_back(Point(s.getEnd().x, s.getEnd().y));
    }

    return points;
}