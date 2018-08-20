#pragma once

#include "ofMain.h"
#include "ofxCv.h"

class Mapamok {
public:
    cv::Mat rvec, tvec;
    ofMatrix4x4 modelMatrix;
    vector<vector<cv::Point3f> > objectPointsCv = vector<vector<cv::Point3f> >(1);
    vector<vector<cv::Point2f> > imagePointsCv = vector<vector<cv::Point2f> >(1);
    ofxCv::Intrinsics intrinsics;
    bool calibrationReady;
    cv::Size2i imageSize;
    
    bool bCV_CALIB_FIX_PRINCIPAL_POINT = false;
    bool bCV_CALIB_FIX_ASPECT_RATIO = true;
    bool bCV_CALIB_FIX_K1 = true;
    bool bCV_CALIB_FIX_K2 = true;
    bool bCV_CALIB_FIX_K3 = true;
    bool bCV_CALIB_ZERO_TANGENT_DIST = false;
    
    void update(int width, int height, vector<ofVec2f>& imagePoints, vector<ofVec3f>& objectPoints) {
        int n = imagePoints.size();
        const static int minPoints = 6;
        if(n < minPoints) {
            calibrationReady = false;
            return;
        }
        vector<cv::Mat> rvecs, tvecs;
        cv::Mat distCoeffs;
        objectPointsCv[0].clear();
        imagePointsCv[0].clear();
        for(int i = 0; i < n; i++) {
            objectPointsCv[0].push_back(ofxCv::toCv(objectPoints[i]));
            imagePointsCv[0].push_back(ofxCv::toCv(imagePoints[i]));
        }
        float aov = 80; // decent guess
        imageSize.width = width;
        imageSize.height = height;
        float f = imageSize.width * ofDegToRad(aov); // this might be wrong, but it's optimized out
        cv::Point2f c = cv::Point2f(imageSize) * (1. / 2);
        cv::Mat1d cameraMatrix = (cv::Mat1d(3, 3) <<
                                  f, 0, c.x,
                                  0, f, c.y,
                                  0, 0, 1);
        int flags = CV_CALIB_USE_INTRINSIC_GUESS;

        if (bCV_CALIB_FIX_PRINCIPAL_POINT) flags |= CV_CALIB_FIX_PRINCIPAL_POINT;
        if (bCV_CALIB_FIX_ASPECT_RATIO) flags |= CV_CALIB_FIX_ASPECT_RATIO;
        if (bCV_CALIB_FIX_K1) flags |= CV_CALIB_FIX_K1;
        if (bCV_CALIB_FIX_K2) flags |= CV_CALIB_FIX_K2;
        if (bCV_CALIB_FIX_K3) flags |= CV_CALIB_FIX_K3;
        if (bCV_CALIB_ZERO_TANGENT_DIST) flags |= CV_CALIB_ZERO_TANGENT_DIST;

        calibrateCamera(objectPointsCv, imagePointsCv, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, flags);
        rvec = rvecs[0];
        tvec = tvecs[0];
        intrinsics.setup(cameraMatrix, imageSize);
        modelMatrix = ofxCv::makeMatrix(rvec, tvec);
        calibrationReady = true;
    }
    void begin() {
        if(calibrationReady) {
            ofPushMatrix();
            ofMatrixMode(OF_MATRIX_PROJECTION);
            ofPushMatrix();
            ofMatrixMode(OF_MATRIX_MODELVIEW);
            intrinsics.loadProjectionMatrix(.1, 7000);
            ofxCv::applyMatrix(modelMatrix);
        }
    }
    void end() {
        if(calibrationReady) {
            ofPopMatrix();
            ofMatrixMode(OF_MATRIX_PROJECTION);
            ofPopMatrix();
            ofMatrixMode(OF_MATRIX_MODELVIEW);
        }
    }
    void save(string filePath){
        
        string dirName = filePath + "/";
        ofDirectory dir(dirName);
        dir.create();
        
        cv::FileStorage fs(ofToDataPath(dirName + "calibration-advanced.yml"), cv::FileStorage::WRITE);

        cv::Mat cameraMatrix = intrinsics.getCameraMatrix();
        fs << "cameraMatrix" << cameraMatrix;
        
        double focalLength = intrinsics.getFocalLength();
        fs << "focalLength" << focalLength;
        
        cv::Point2d fov = intrinsics.getFov();
        fs << "fov" << fov;
        
        cv::Point2d principalPoint = intrinsics.getPrincipalPoint();
        fs << "principalPoint" << principalPoint;
        
        cv::Size imageSize = intrinsics.getImageSize();
        fs << "imageSize" << imageSize;
        
        fs << "translationVector" << tvec;
        fs << "rotationVector" << rvec;
        
        cv::Mat rotationMatrix;
        Rodrigues(rvec, rotationMatrix);
        fs << "rotationMatrix" << rotationMatrix;
        
        double rotationAngleRadians = norm(rvec, cv::NORM_L2);
        double rotationAngleDegrees = ofRadToDeg(rotationAngleRadians);
        cv::Mat rotationAxis = rvec / rotationAngleRadians;
        fs << "rotationAngleRadians" << rotationAngleRadians;
        fs << "rotationAngleDegrees" << rotationAngleDegrees;
        fs << "rotationAxis" << rotationAxis;
        
        ofVec3f axis(rotationAxis.at<double>(0), rotationAxis.at<double>(1), rotationAxis.at<double>(2));
        ofVec3f euler = ofQuaternion(rotationAngleDegrees, axis).getEuler();
        cv::Mat eulerMat = (cv::Mat_<double>(3,1) << euler.x, euler.y, euler.z);
        fs << "euler" << eulerMat;

        fs << "objectPoints" << cv::Mat(objectPointsCv[0]);
        fs << "imagePoints" << cv::Mat(imagePointsCv[0]);

    }
    void load(string filePath){

        cv::FileStorage fs(ofToDataPath(filePath + "/calibration-advanced.yml", true), cv::FileStorage::READ);
        
        cv::Mat objPointsMat, imgPointsMat;
        fs["objectPoints"] >> objPointsMat;
        fs["imagePoints"] >> imgPointsMat;

        int numVals;
        float x, y, z;
        cv::Point3f oP;
        
        const float* objVals = objPointsMat.ptr<float>(0);
        numVals = objPointsMat.cols * objPointsMat.rows;
        
        objectPointsCv[0].clear();

        for(int i = 0; i < numVals; i+=3) {
            oP.x = objVals[i];
            oP.y = objVals[i+1];
            oP.z = objVals[i+2];
            objectPointsCv[0].push_back(oP);
        }
        
        cv::Point2f iP;
        
        const float* imgVals = imgPointsMat.ptr<float>(0);
        numVals = objPointsMat.cols * objPointsMat.rows;
        
        imagePointsCv[0].clear();

        for(int i = 0; i < numVals; i+=2) {
            iP.x = imgVals[i];
            iP.y = imgVals[i+1];
            imagePointsCv[0].push_back(iP);
        }
        
        cv::Mat cameraMatrix;
        cv::Size2i imageSize;
        fs["cameraMatrix"] >> cameraMatrix;
        fs["imageSize"][0] >> imageSize.width;
        fs["imageSize"][1] >> imageSize.height;
        fs["rotationVector"] >> rvec;
        fs["translationVector"] >> tvec;
        
        intrinsics.setup(cameraMatrix, imageSize);
        modelMatrix = ofxCv::makeMatrix(rvec, tvec);
        
        calibrationReady = true;
        
    }
};
