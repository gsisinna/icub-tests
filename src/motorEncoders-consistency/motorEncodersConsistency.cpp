/*
 * iCub Robot Unit Tests (Robot Testing Framework)
 *
 * Copyright (C) 2015-2019 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <math.h>
#include <robottestingframework/TestAssert.h>
#include <robottestingframework/dll/Plugin.h>
#include <yarp/os/Time.h>
#include <yarp/os/LogStream.h>
#include <yarp/math/Math.h>
#include <yarp/os/Property.h>
#include <yarp/os/ResourceFinder.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include "motorEncodersConsistency.h"
#include <iostream>
#include <yarp/dev/IRemoteVariables.h>

using namespace std;

//example     -v -t OpticalEncodersConsistency.dll -p "--robot icub --part left_arm --joints ""(0 1 2)"" --home ""(-30 30 10)"" --speed ""(20 20 20)"" --max ""(-20 40 20)"" --min ""(-40 20 0)"" --cycles 10 --tolerance 1.0 "
//example2    -v -t OpticalEncodersConsistency.dll -p "--robot icub --part head     --joints ""(2)""     --home ""(0)""         --speed ""(20      )"" --max ""(10      )""  --min ""(-10)""      --cycles 10 --tolerance 1.0 "
//-v - s "C:\software\icub-tests\suites\encoders-icubSim.xml"
using namespace robottestingframework;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::math;

// prepare the plugin
ROBOTTESTINGFRAMEWORK_PREPARE_PLUGIN(OpticalEncodersConsistency)

OpticalEncodersConsistency::OpticalEncodersConsistency() : yarp::robottestingframework::TestCase("OpticalEncodersConsistency") {
    jointsList=0;
    dd=0;
    ipos=0;
    icmd=0;
    iimd=0;
    ienc=0;
    imot=0;
    imotenc=0;

    enc_jnt=0;
    enc_jnt2mot=0;
    enc_mot=0;
    vel_jnt=0;
    vel_jnt2mot=0;
    vel_mot=0;
    acc_jnt=0;
    acc_jnt2mot=0;
    acc_mot=0;
    cycles =10;
    tolerance = 1.0;
    plot_enabled = false;
}

OpticalEncodersConsistency::~OpticalEncodersConsistency() { }

bool OpticalEncodersConsistency::setup(yarp::os::Property& property) {

    if(property.check("name"))
        setName(property.find("name").asString());

    char b[5000];
    strcpy (b,property.toString().c_str());
    ROBOTTESTINGFRAMEWORK_TEST_REPORT("on setup()");
    // updating parameters
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("robot"), "The robot name must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("part"), "The part name must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("joints"), "The joints list must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("home"),      "The home position must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("max"),       "The max position must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("min"),       "The min position must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("speed"),     "The positionMove reference speed must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("tolerance"), "The tolerance of the control signal must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("matrix_size"),  "The matrix size must be given!");
   // ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("matrix"),       "The coupling matrix must be given!");
    robotName = property.find("robot").asString();
    partName = property.find("part").asString();
    if(property.check("plot_enabled"))
        plot_enabled = property.find("plot_enabled").asBool();
    /*if(plot_enabled)
    {
        plotString1 = property.find("plotString1").asString();
        plotString2 = property.find("plotString2").asString();
        plotString3 = property.find("plotString3").asString();
        plotString4 = property.find("plotString4").asString();
    }*/
    Bottle* jointsBottle = property.find("joints").asList();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(jointsBottle!=0,"unable to parse joints parameter");

    Bottle* homeBottle = property.find("home").asList();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(homeBottle!=0,"unable to parse home parameter");

    Bottle* maxBottle = property.find("max").asList();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(maxBottle!=0,"unable to parse max parameter");

    Bottle* minBottle = property.find("min").asList();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(minBottle!=0,"unable to parse min parameter");

    Bottle* speedBottle = property.find("speed").asList();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(speedBottle!=0,"unable to parse speed parameter");

    tolerance = property.find("tolerance").asFloat64();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(tolerance>=0,"invalid tolerance");

    int matrix_size=property.find("matrix_size").asInt32();
    if (matrix_size>0)
    {
        matrix.resize(matrix_size,matrix_size);
        matrix.eye();

        // The couplig matrix is retrived run-time by the IRemoteVariable interface accessing the jinimatic_mj variable
        //
        // Bottle* matrixBottle = property.find("matrix").asList();

        // if (matrixBottle!= NULL && matrixBottle->size() == (matrix_size*matrix_size) )
        // {
        //     for (int i=0; i< (matrix_size*matrix_size); i++)
        //     {
        //         matrix.data()[i]=matrixBottle->get(i).asFloat64();
        //     }
        // }
        // else
        // {
        //    char buff [500];
        //    sprintf (buff, "invalid number of elements of parameter matrix %d!=%d", matrixBottle->size() , (matrix_size*matrix_size));
        //    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(buff);
        // }
    }
    else
    {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("invalid matrix_size: must be >0");
    }

    //optional parameters
    if (property.check("cycles"))
    {cycles = property.find("cycles").asInt32();}

    Property options;
    options.put("device", "remote_controlboard");
    options.put("remote", "/"+robotName+"/"+partName);
    options.put("local", "/positionDirectTest/"+robotName+"/"+partName);

    dd = new PolyDriver(options);
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->isValid(),"Unable to open device driver");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(ienc),"Unable to open encoders interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(ipos),"Unable to open position interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(icmd),"Unable to open control mode interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(iimd),"Unable to open interaction mode interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(imotenc),"Unable to open motor encoders interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(imot),"Unable to open motor interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(ivar),"Unable to open remote variables interface");


    if (!ienc->getAxes(&n_part_joints))
    {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("unable to get the number of joints of the part");
    }

    int n_cmd_joints = jointsBottle->size();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(n_cmd_joints>0 && n_cmd_joints<=n_part_joints,"invalid number of joints, it must be >0 & <= number of part joints");
    jointsList.clear();
    for (int i=0; i <n_cmd_joints; i++) jointsList.push_back(jointsBottle->get(i).asInt32());

    enc_jnt.resize(n_cmd_joints); enc_jnt.zero();
    enc_mot.resize(n_cmd_joints); enc_mot.zero();
    vel_jnt.resize(n_cmd_joints); vel_jnt.zero();
    vel_mot.resize(n_cmd_joints); vel_mot.zero();
    acc_jnt.resize(n_cmd_joints); acc_jnt.zero();
    acc_mot.resize(n_cmd_joints); acc_mot.zero();
    prev_enc_jnt.resize(n_cmd_joints); prev_enc_jnt.zero();
    prev_enc_mot.resize(n_cmd_joints); prev_enc_mot.zero();
    prev_enc_jnt2mot.resize(n_cmd_joints); prev_enc_jnt2mot.zero();
    prev_vel_jnt.resize(n_cmd_joints); prev_vel_jnt.zero();
    prev_vel_mot.resize(n_cmd_joints); prev_vel_mot.zero();
    prev_vel_jnt2mot.resize(n_cmd_joints); prev_vel_jnt2mot.zero();
    prev_acc_jnt.resize(n_cmd_joints); prev_acc_jnt.zero();
    prev_acc_mot.resize(n_cmd_joints); prev_acc_mot.zero();
    prev_acc_jnt2mot.resize(n_cmd_joints); prev_acc_jnt2mot.zero();
    zero_vector.resize(n_cmd_joints);
    zero_vector.zero();

    max.resize(n_cmd_joints);     for (int i=0; i< n_cmd_joints; i++) max[i]=maxBottle->get(i).asFloat64();
    min.resize(n_cmd_joints);     for (int i=0; i< n_cmd_joints; i++) min[i]=minBottle->get(i).asFloat64();
    home.resize(n_cmd_joints);    for (int i=0; i< n_cmd_joints; i++) home[i]=homeBottle->get(i).asFloat64();
    speed.resize(n_cmd_joints);   for (int i=0; i< n_cmd_joints; i++) speed[i]=speedBottle->get(i).asFloat64();
    gearbox.resize(n_cmd_joints);
    for (int i=0; i< n_cmd_joints; i++)
    {
        double t;
        int b=imot->getGearboxRatio(jointsList[i],&t);
        gearbox[i]=t;
    }


    return true;
}

void OpticalEncodersConsistency::tearDown()
{
    char buff[500];
    sprintf(buff,"Closing test module");ROBOTTESTINGFRAMEWORK_TEST_REPORT(buff);
    setMode(VOCAB_CM_POSITION);
    goHome();
    if (dd) {delete dd; dd =0;}
}

void OpticalEncodersConsistency::setMode(int desired_mode)
{
    if (icmd == 0) ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Invalid control mode interface");
    if (iimd == 0) ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Invalid interaction mode interface");

    for (unsigned int i=0; i<jointsList.size(); i++)
    {
        icmd->setControlMode((int)jointsList[i],desired_mode);
        iimd->setInteractionMode((int)jointsList[i],VOCAB_IM_STIFF);
        yarp::os::Time::delay(0.010);
    }

    int cmode;
    yarp::dev::InteractionModeEnum imode;
    int timeout = 0;

    while (1)
    {
        int ok=0;
        for (unsigned int i=0; i<jointsList.size(); i++)
        {
            icmd->getControlMode ((int)jointsList[i],&cmode);
            iimd->getInteractionMode((int)jointsList[i],&imode);
            if (cmode==desired_mode && imode==VOCAB_IM_STIFF) ok++;
        }
        if (ok==jointsList.size()) break;
        if (timeout>100)
        {
            ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Unable to set control mode/interaction mode");
        }
        yarp::os::Time::delay(0.2);
        timeout++;
    }
}

void OpticalEncodersConsistency::goHome()
{
    if (ipos == 0) ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Invalid position control interface");
    if (ienc == 0) ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Invalid encoders interface");

    bool ret = true;
    char buff [500];
    sprintf(buff,"Homing the whole part");ROBOTTESTINGFRAMEWORK_TEST_REPORT(buff);

    for (unsigned int i=0; i<jointsList.size(); i++)
    {
        ret = ipos->setRefSpeed((int)jointsList[i],speed[i]);
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ret, "ipos->setRefSpeed returned false");
        ret = ipos->positionMove((int)jointsList[i],home[i]);
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ret, "ipos->positionMove returned false");
    }

    int timeout = 0;
    while (1)
    {
        int in_position=0;
        for (unsigned int i=0; i<jointsList.size(); i++)
        {
            double tmp=0;
            ienc->getEncoder((int)jointsList[i],&tmp);
            if (fabs(tmp-home[i])<tolerance) in_position++;
        }
        if (in_position==jointsList.size()) break;
        if (timeout>100)
        {
            ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Timeout while reaching home position");
        }
        yarp::os::Time::delay(0.2);
        timeout++;
    }
}

void OpticalEncodersConsistency::saveToFile(std::string filename, yarp::os::Bottle &b)
{
    std::fstream fs;
    fs.open (filename.c_str(), std::fstream::out);

    for (int i=0; i<b.size(); i++)
    {
        std::string s = b.get(i).toString();
        std::replace(s.begin(), s.end(), '(', ' ');
        std::replace(s.begin(), s.end(), ')', ' ');
        fs << s << endl;
    }

    fs.close();
}

void OpticalEncodersConsistency::run()
{
    char buff [500];
    setMode(VOCAB_CM_POSITION);
    goHome();

    bool go_to_max=false;
    for (unsigned int i=0; i<jointsList.size(); i++)
    {
        ipos->positionMove((int)jointsList[i], min[i]);
    }

    int  cycle=0;
    double start_time = yarp::os::Time::now();

    //****************************************************************************************
    //Retrieving coupling matrix using IRemoteVariable
    //****************************************************************************************

    yarp::os::Bottle b;

    ivar->getRemoteVariable("kinematic_mj", b);

    int matrix_size = matrix.cols();

    matrix.eye();
    
    int njoints [4];
    
    for(int i=0 ; i< b.size() ; i++)
    {
        Bottle bv;
        bv.fromString(b.get(i).toString());
        njoints[i] = sqrt(bv.size());

        int ele = 0;
        if(i==0) {
        for (int r=0; r < njoints[i]; r++) 
        {
            for (int c=0; c < njoints[i]; c++) 
            {
                matrix(r,c) = bv.get(ele).asFloat64();
                ele++;
            }
        }

       }  
       else{
           for (int r=0; r < njoints[i]; r++) 
            {
                for (int c=0; c < njoints[i]; c++) 
                {
                    int jntprev = 0;
                    for (int j=0; j < i; j++) jntprev += njoints[j];
                    if(!jntprev > matrix_size)  matrix(r+jntprev,c+jntprev) = bv.get(ele).asFloat64();
                    ele++;
                }
            }
       }
    
    }

    // yDebug() << "MATRIX J2M : \n" << matrix.toString();

// **************************************************************************************

    trasp_matrix = matrix.transposed();
    inv_matrix = yarp::math::luinv(matrix);
    inv_trasp_matrix = inv_matrix.transposed();

    sprintf(buff,"Matrix:\n %s \n", matrix.toString().c_str());
    ROBOTTESTINGFRAMEWORK_TEST_REPORT(buff);
    sprintf(buff,"Inv matrix:\n %s \n", inv_matrix.toString().c_str());
    ROBOTTESTINGFRAMEWORK_TEST_REPORT(buff);

    Bottle dataToPlot_test1;
    Bottle dataToPlot_test2;
    Bottle dataToPlot_test3;
    Bottle dataToPlot_test4;
    Bottle dataToPlot_test1rev;

    bool test_data_is_valid = false;
    bool first_time = true;
    yarp::sig::Vector off_enc_mot; off_enc_mot.resize(jointsList.size());
    yarp::sig::Vector off_enc_jnt; off_enc_jnt.resize(jointsList.size());
    yarp::sig::Vector off_enc_jnt2mot; off_enc_jnt2mot.resize(jointsList.size());
    yarp::sig::Vector off_enc_mot2jnt; off_enc_mot2jnt.resize(jointsList.size());
    yarp::sig::Vector tmp_vector;
    tmp_vector.resize(n_part_joints);



    while (1)
    {
        double curr_time = yarp::os::Time::now();
        double elapsed = curr_time - start_time;

        bool ret = true;
        ret = ienc->getEncoders(tmp_vector.data());
        for (unsigned int i = 0; i < jointsList.size(); i++)
            enc_jnt[i] = tmp_vector[jointsList[i]];


        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ret, "ienc->getEncoders returned false");
        ret = imotenc->getMotorEncoders(tmp_vector.data());             for (unsigned int i = 0; i < jointsList.size(); i++) enc_mot[i] = tmp_vector[jointsList(i)];
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ret, "imotenc->getMotorEncoder returned false");
        ret = ienc->getEncoderSpeeds(tmp_vector.data());             for (unsigned int i = 0; i < jointsList.size(); i++) vel_jnt[i] = tmp_vector[jointsList(i)];
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ret, "ienc->getEncoderSpeeds returned false");
        ret = imotenc->getMotorEncoderSpeeds(tmp_vector.data());        for (unsigned int i = 0; i < jointsList.size(); i++) vel_mot[i] = tmp_vector[jointsList(i)];
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ret, "imotenc->getMotorEncoderSpeeds returned false");
        ret = ienc->getEncoderAccelerations(tmp_vector.data());      for (unsigned int i = 0; i < jointsList.size(); i++) acc_jnt[i] = tmp_vector[jointsList(i)];
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ret, "ienc->getEncoderAccelerations returned false");
        ret = imotenc->getMotorEncoderAccelerations(tmp_vector.data()); for (unsigned int i = 0; i < jointsList.size(); i++) acc_mot[i] = tmp_vector[jointsList(i)];
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(ret, "imotenc->getMotorEncoderAccelerations returned false");

        //if (enc_jnt == zero_vector) { ROBOTTESTINGFRAMEWORK_TEST_REPORT("Invalid getEncoders data"); test_data_is_valid = true; }
        //if (enc_mot == zero_vector) { ROBOTTESTINGFRAMEWORK_TEST_REPORT("Invalid getMotorEncoders data"); test_data_is_valid = true; }
        //if (vel_jnt == zero_vector) { ROBOTTESTINGFRAMEWORK_TEST_REPORT("Invalid getEncoderSpeeds data"); test_data_is_valid = true; }
        //if (vel_mot == zero_vector) { ROBOTTESTINGFRAMEWORK_TEST_REPORT("Invalid getMotorEncoderSpeeds data"); test_data_is_valid = true; }
        //if (acc_jnt == zero_vector) { ROBOTTESTINGFRAMEWORK_TEST_REPORT("Invalid getEncoderAccelerations data"); test_data_is_valid = true; }
        //if (acc_mot == zero_vector) { ROBOTTESTINGFRAMEWORK_TEST_REPORT("Invalid getMotorEncoderAccelerations data"); test_data_is_valid = true; }

        if (first_time)
        {
            off_enc_jnt = enc_jnt;
            off_enc_mot2jnt = enc_mot2jnt;

        }
        
        enc_jnt2mot = matrix * enc_jnt;
        enc_mot2jnt = inv_matrix * (enc_mot - off_enc_mot);
        vel_jnt2mot = matrix * vel_jnt;
        //acc_jnt2mot = matrix * acc_jnt;


        for (unsigned int i = 0; i < jointsList.size(); i++) enc_jnt2mot[i] = enc_jnt2mot[i] * gearbox[i];;
        for (unsigned int i = 0; i < jointsList.size(); i++) vel_jnt2mot[i] = vel_jnt2mot[i] * gearbox[i];
        //for (unsigned int i = 0; i < jointsList.size(); i++) acc_jnt2mot[i] = acc_jnt2mot[i] * gearbox[i];
        for (unsigned int i = 0; i < jointsList.size(); i++) enc_mot2jnt[i] = enc_mot2jnt[i] / gearbox[i];

        bool reached = false;
        int in_position = 0;
        for (unsigned int i = 0; i < jointsList.size(); i++)
        {
            double curr_val = 0;
            if (go_to_max == false) curr_val = min[i];
            else                  curr_val = max[i];
            if (fabs(enc_jnt[i] - curr_val) < tolerance) in_position++;
        }
        if (in_position == jointsList.size()) reached = true;

        if (elapsed >= 20.0)
        {
            ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Timeout while moving joint");
        }

        if (reached)
        {
            sprintf(buff, "Test cycle %d/%d", cycle, cycles); ROBOTTESTINGFRAMEWORK_TEST_REPORT(buff);
            if (go_to_max == false)
            {
                for (unsigned int i = 0; i < jointsList.size(); i++)
                    ipos->positionMove(jointsList[i], max[i]);
                go_to_max = true;
                cycle++;
                start_time = yarp::os::Time::now();
            }
            else
            {
                for (unsigned int i = 0; i < jointsList.size(); i++)
                    ipos->positionMove(jointsList[i], min[i]);
                go_to_max = false;
                cycle++;
                start_time = yarp::os::Time::now();
            }
        }

        //update previous and computes diff
        diff_enc_jnt = (enc_jnt - prev_enc_jnt) / 0.010;
        diff_enc_mot = (enc_mot - prev_enc_mot) / 0.010;
        diff_enc_jnt2mot = (enc_jnt2mot - prev_enc_jnt2mot) / 0.010;
        diff_vel_jnt = (vel_jnt - prev_vel_jnt) / 0.010;
        diff_vel_mot = (vel_mot - prev_vel_mot) / 0.010;
        diff_vel_jnt2mot = (vel_jnt2mot - prev_vel_jnt2mot) / 0.010;
        diff_acc_jnt = (acc_jnt - prev_acc_jnt) / 0.010;
        diff_acc_mot = (acc_mot - prev_acc_mot) / 0.010;
        //diff_acc_jnt2mot = (acc_jnt2mot - prev_acc_jnt2mot) / 0.010;
        prev_enc_jnt = enc_jnt;
        prev_enc_mot = enc_mot;
        prev_enc_jnt2mot = enc_jnt2mot;
        prev_vel_jnt = vel_jnt;
        prev_vel_mot = vel_mot;
        prev_vel_jnt2mot = vel_jnt2mot;
       // prev_acc_jnt = acc_jnt;
        //prev_acc_mot = acc_mot;
       // prev_acc_jnt2mot = acc_jnt2mot;

        if (first_time)
        {
            off_enc_mot = enc_mot;
            off_enc_jnt2mot = enc_jnt2mot;
        }

        {
            //prepare data to plot
            //JOINT POSITIONS vs MOTOR POSITIONS
            Bottle& row_test1 = dataToPlot_test1.addList();
            Bottle& v1_test1 = row_test1.addList();
            Bottle& v2_test1 = row_test1.addList();
            yarp::sig::Vector v1 = enc_mot - off_enc_mot;
            yarp::sig::Vector v2 = enc_jnt2mot - off_enc_jnt2mot;
            v1_test1.read(v1);
            v2_test1.read(v2);
        }

        {
            //JOINT VELOCITES vs MOTOR VELOCITIES
            Bottle& row_test2 = dataToPlot_test2.addList();
            Bottle& v1_test2 = row_test2.addList();
            Bottle& v2_test2 = row_test2.addList();
            v1_test2.read(vel_mot);
            v2_test2.read(vel_jnt2mot);
        }

        {
            //JOINT POSITIONS(DERIVED) vs JOINT SPEED
            if (first_time == false)
            {
                Bottle& row_test3 = dataToPlot_test3.addList();
                Bottle& v1_test3 = row_test3.addList();
                Bottle& v2_test3 = row_test3.addList();
                v1_test3.read(vel_jnt);
                v2_test3.read(diff_enc_jnt);
            }
        }

        {
            //MOTOR POSITIONS(DERIVED) vs MOTOR SPEED
            if (first_time == false)
            {
                Bottle& row_test4 = dataToPlot_test4.addList();
                Bottle& v1_test4 = row_test4.addList();
                Bottle& v2_test4 = row_test4.addList();
                v1_test4.read(vel_mot);
                v2_test4.read(diff_enc_mot);
            }
        }

        {
            //JOINT POSITIONS vs MOTOR POSITIONS REVERSED
            Bottle& row_test1 = dataToPlot_test1rev.addList();
            Bottle& v1_test1 = row_test1.addList();
            Bottle& v2_test1 = row_test1.addList();
            yarp::sig::Vector v1 = enc_jnt;
            yarp::sig::Vector v2 = enc_mot2jnt + off_enc_jnt;
            v1_test1.read(v1);
            v2_test1.read(v2);
        }

        //flag set
        first_time = false;

        //exit condition
        if (cycle>=cycles) break;
    }

    goHome();

    yarp::os::ResourceFinder rf;
    rf.setDefaultContext("scripts");

    string partfilename = partName+".txt";
    string testfilename = "encConsis_";
    string filename1 = testfilename + "jointPos_MotorPos_" + partfilename;
    saveToFile(filename1,dataToPlot_test1);
    string filename2 = testfilename + "jointVel_motorVel_" + partfilename;
    saveToFile(filename2,dataToPlot_test2);
    string filename3 = testfilename + "joint_derivedVel_vel_" + partfilename;
    saveToFile(filename3,dataToPlot_test3);
    string filename4 = testfilename + "motor_derivedVel_vel_" + partfilename;
    saveToFile(filename4,dataToPlot_test4);

    string filename1rev = testfilename + "jointPos_MotorPos_reversed_" + partfilename;
    saveToFile(filename1rev,dataToPlot_test1rev);

    //find octave scripts
    std::string octaveFile = rf.findFile("encoderConsistencyPlotAll.m");
    if(octaveFile.size() == 0)
    {
        yError()<<"Cannot find file encoderConsistencyPlotAll.m";
        return;
    }

    //prepare octave command
    std::string octaveCommand= "octave --path "+ getPath(octaveFile);
    stringstream ss;
    ss << jointsList.size();
    string str = ss.str();
    octaveCommand+= " -q --eval \"encoderConsistencyPlotAll('" +partName +"'," + str +")\"  --persist";

    if(plot_enabled)
    {
        int ret = system (octaveCommand.c_str());
    }
    else
    {
         yInfo() << "Test has collected all data. You need to plot data to check is test is passed";
         yInfo() << "Please run following command to plot data.";
         yInfo() << octaveCommand;
         yInfo() << "To exit from Octave application please type 'exit' command.";
    }
   // ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(test_data_is_valid,"Invalid data obtained from encoders interface");
}


std::string OpticalEncodersConsistency::getPath(const std::string& str)
{
  size_t found;
  found=str.find_last_of("/\\");
  return(str.substr(0,found));
}
