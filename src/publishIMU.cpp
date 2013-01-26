#include "ros/ros.h"
#include "std_msgs/String.h"
#include <IMU/spatialRaw.h>
#include "spatial_helper.h"
#include "config.h"
extern pthread_mutex_t mutex; 	//used when handling data q
extern pthread_cond_t cond;		//used when handling data  q
using namespace std;

void copySpatialRaw(spatial::PhidgetRawDataQ::iterator it, IMU::spatialRaw *msg)	{

	//Copying Timestamp
	msg->timestamp.sec = it->timestamp.seconds;
	msg->timestamp.nsec = it->timestamp.microseconds;

	//Copying Acceleration (in g's)
	msg->a_x = it->acceleration[0];
	msg->a_y = it->acceleration[1];
	msg->a_z = it->acceleration[2];

	//Copying Angular Rate (in deg/s)
	msg->w_x = it->angularRate[0];
	msg->w_y = it->angularRate[1];
	msg->w_z = it->angularRate[2];

	//Copying Magnetic field (in gauss)
	msg->m_x = it->magneticField[0];
	msg->m_y = it->magneticField[1];
	msg->m_z = it->magneticField[2];

}

void fillSpatialMsg(spatial::PhidgetRawDataQ::iterator it, spatial::PhidgetRawDataQ* dataQueue, IMU::spatialRaw *msg)	{
	ROS_INFO("Filling spatialMsg");
	//Dealing with data Queue
	pthread_mutex_lock(&mutex);

	//Check to see if dataQueue is empty. 
	//If it is, wait.
	if(dataQueue->empty())	{
		pthread_cond_wait(&cond, &mutex);
	}
	 
	
	ROS_INFO("DataQ not empty!");
	it = dataQueue->begin();
	copySpatialRaw(it, msg);
	dataQueue->pop_front();
	pthread_mutex_unlock(&mutex);


}

int main(int argc, char* argv[]){

  	int ROSbufferSize = 100, ROScount = 0;
 	ros::init(argc, argv, "Phidget_Stuff");
  	ros::NodeHandle PhidgetNode;
  	ros::Publisher PhidgetPub = 
    	PhidgetNode.advertise<IMU::spatialRaw>("IMU_data", ROSbufferSize);
  	ros::Rate loop_rate(10);
	
	//Creating/Initializing Spatial Handle
	//------------------------------------
	CPhidgetSpatialHandle spatial =0;
	CPhidgetSpatial_create(&spatial);

	//Setting up data q
	//------------------------------------
	spatial::PhidgetRawDataQ* dataQueue = new spatial::PhidgetRawDataQ();
	spatial::PhidgetRawDataQ::iterator it = dataQueue->begin();
	
	//init mutex
	//-------------------------------------
	if(pthread_mutex_init(&mutex, NULL)!= 0)	{
		ROS_INFO("mutex init failed");
		return -1;	//erm this is bad
	}
	else	{
		ROS_INFO("mutex init success \n");
	}

	//init cond
	//-----------------------------------
	if(pthread_cond_init(&cond, NULL)!=0)	{
		ROS_INFO("cond init failed!");
	}
	else	{
		ROS_INFO("cond init success \n");
	}
	
	//set up spatial
	//-------------------------------------
	spatial::spatial_setup(spatial, dataQueue, DATA_RATE);

while(1)	{
 // 	while(ros::ok()) {
		
		ROS_INFO("Loop!");

		//Filling up 
		IMU::spatialRaw  msg;
			
		fillSpatialMsg(it, dataQueue, &msg);

		ROS_INFO("Time %ds %dns", msg.timestamp.sec, msg.timestamp.nsec);
		ROS_INFO("Gyr X:%f Y:%f Z:%f", msg.w_x, msg.w_y, msg.w_z);	
//		PhidgetPub.publish(msg);
	

    	ROScount++;
		ROS_INFO("Going to sleep");
//		loop_rate.sleep();
    	ros::spinOnce();
 	}
}
