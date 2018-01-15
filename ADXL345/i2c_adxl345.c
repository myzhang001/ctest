#include <stdio.h>
//#include "i2c_adxl345_1.h" //---use ioctl() to operate I2C
#include "i2c_adxl345_2.h" //--- use  read() write() to operate I2C
#include "mathwork.h"
#include "filters.h"
#include "data_server.h"

#define TCP_TRANSFER

int main(void)
{
   int ret_val;
   int i,j,k;
   uint8_t dat;
   uint8_t addr;
   int16_t accXYZ[3]={0}; // acceleration value of XYZ 
   double  faccXYZ[3]; //faccXYZ=fs*accXYZ
   uint8_t xyz[6]={0};
   int16_t *tmp=(int16_t *)xyz;
   double fs=3.9/1000.0;//scale factor 4mg/LSB for full resolutioin, 
   //----- filter data base -----
   struct int16MAFilterDB fdb_accXYZ[3];
   //Note: Big limit value smooths data better,but you have to trade off with reactive speed.
   int16_t  relative_int16limit=128;//256 //1.0*1000/3.9=256; relative difference limit between two fdb_faccXYZ[] data

   //------ time value ----
   struct timeval tm_start,tm_end;
   int send_count=0;

   //----- open and setup i2c slave
   printf("init i2c slave...\n");
   if( init_I2C_Slave() <0 )
   {
	printf("init i2c slave failed!\n");
        ret_val=-1;
	goto INIT_I2C_FAIL;
   }

   //------ set up ADXL345
   init_ADXL345(ADXL_ODR_400HZ,ADXL_RANGE_4G);

   //---- init filter data base
   printf("Init int16MA filter data base ...\n");
   if( Init_int16MAFilterDB_NG(3, fdb_accXYZ, 2, relative_int16limit)<0) //2^2=4 points average filter
   {
	ret_val=-2;
        goto INIT_MAFILTER_FAIL;
    }

  //---- preare TCP data server
#ifdef TCP_TRANSFER
   printf("Prepare TCP data server ...\n");
   if(prepare_data_server() < 0)
   {
        printf(" fail to prepare data server!\n");
        ret_val=-3;
        goto CALL_FAIL;
   }

   //----- wait to accept data client ----
   printf("wait for Matlab to connect...\n");
   cltsock_desc=accept_data_client();
   if(cltsock_desc < 0)
   {
        printf(" Fail to accept data client!\n");
	ret_val=-4;
        goto CALL_FAIL;
   }
#endif  //----- TCP TRANSER


#if 1  //------------ I2C operation with read() /write() -------------
   i=0;
   gettimeofday(&tm_start,NULL);
   while(1) //(i<100000)
   {
       i++;
       get_int16XYZ_ADXL345(accXYZ);

       //-----applay int16 Moving Average Filter 
       int16_MAfilter_NG(3,fdb_accXYZ, accXYZ, accXYZ, 0); // three filter groups, only [0] data filterd for each filter
       //----- now accXYZ is with filtered data, spiking values have been trimmed.

       //------- use factor
       for(j=0;j<3;j++)
       {
		faccXYZ[j]=fs*accXYZ[j];
       		if(faccXYZ[j]>1.5)
			printf("\n alarm: faccXYZ[%d]=%f \n",j,faccXYZ[j]);
       }
       printf("I=%d, accX: %f,  accY: %f,  accZ:%f \r", i, faccXYZ[0], faccXYZ[1], faccXYZ[2]);
       fflush(stdout);

#ifdef TCP_TRANSFER  //--- every 10th 
       if(send_count==0)
       {
               if( send_client_data((uint8_t *)faccXYZ,3*sizeof(double)) < 0)
                       printf("-------- fail to send client data ------\n");
                send_count=10;
        }
        else
                send_count-=1;
#endif
   } // end of while()
   gettimeofday(&tm_end,NULL);
   printf("\n Time elapsed: %fs \n",get_costtimeus(tm_start,tm_end)/1000000.0);

#endif  //--------- end I2c read()/write() --------




#if 0 //------------  I2C OPERATION with IOCTL() -----------------
   printf("read register...\n");
   addr=0x00;
   Single_Read_ADXL345(addr, &dat);
   printf("dat=0x%02x\n",dat);

   Single_Write_ADXL345(0x2c,0x0e);
   addr=0x2c;
   Single_Read_ADXL345(addr, &dat);
   printf("dat=0x%02x\n",dat);

   i=0;
   addr=0x32;

   for(i=0x1D; i<0x39; i++)
   {
	Single_Read_ADXL345(i, &dat);
        printf("ADDR 0x%02x    VAL 0x%02x \n",i,dat);
   }

   while(1) 
   {
	i++;
	//----- !!!!! wait for data, if data is NOT ready, Multi_Read() will crash Openwrt !!!!!
	while(!IS_DataReady_ADXL345()) {
		usleep(100000);
		printf(" data not ready! \n");
	}
	//----- read data
   	Multi_Read_ADXL345(6,addr,(uint8_t *)accXYZ);
//   	printf("accX=%d, accY=%d, accZ=%d \n",accXYZ[0],accXYZ[1],accXYZ[2]);
   	printf("I=%d   accX=%f, accY=%f, accZ=%f \r",i,accXYZ[0]*fs,accXYZ[1]*fs,accXYZ[2]*fs);
	fflush(stdout);
        usleep(10000);//for ODR=100HZ, sleep 5ms
   }

#endif  //-------- end of I2C operation with ioctl() -----




CALL_FAIL:


INIT_MAFILTER_FAIL:
   //---- release filter data base
   Release_int16MAFilterDB_NG(3,fdb_accXYZ);

   //----- close i2c dev
   close(g_I2Cfd);

INIT_I2C_FAIL:
   return ret_val;
}
