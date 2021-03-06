#include "demodulation.h"

//传入udp_recv下的CHdata即可解调
Demodulation::Demodulation(UDP_Recv* udp_Recv)
{
    udp_recv = udp_Recv;

    DEMOdata_flash = make_shared<CirQueue<float>>(LenoDemo);
    DEMOdata_save = make_shared<CirQueue<float>>(LenoDemoSave);

    //    demo_CHdata = new unsigned char[READ_LENGTH](); //动态数组初始化
    //    Vi = new float[CHDATA_LENGTH]();
    //    Vq = new float[CHDATA_LENGTH]();
    //    Ph = new float[CHDATA_LENGTH]();
    //    atanTable = new float[NUMTABLE]();
    //    demo_CHdata_DEC_all = new int[CHDATA_ALL_LENGTH]();
    //    demo_CHdata_DEC_1 = new int[CHDATA_LENGTH]();
    //    demo_CHdata_DEC_2 = new int[CHDATA_LENGTH]();
    //    demo_CHdata_DEC_3 = new int[CHDATA_LENGTH]();
    //    demo_CHdata_DEC_4 = new int[CHDATA_LENGTH]();

    readConfigFile();

    RealPh = new float[peakNum]();
    PriorPh = new float[peakNum]();
    K=new float[peakNum]();
    PriorK=new float[peakNum]();

    readAtanTable(atanTable);
}

void Demodulation::readConfigFile()
{
    QString filePath = QString("C:/Qt_UDP_DAS/peak.txt"); //build所在目录下
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Can't open the Configration file!"<<endl;
    }
    QByteArray configData = file.readAll(); //读取所有数据
    char peakNumChar_3 = configData[3]; //peakNum的百位存放在第四个位置
    char peakNumChar_4 = configData[4]; //peakNum的十位存放在第五个位置
    char peakNumChar_5 = configData[5]; //peakNum的个位存放在第六个位置
    char peakNumChar_6 = configData[6]; //换行符
//    peakNum = 48;


    //peakNum是个位数
    if(peakNumChar_4 == '\n'){
        bool ok;
        peakNum = QString(peakNumChar_3).toInt(&ok,16)*1;
    }
    //peakNum是十位数
    else if(peakNumChar_5 == '\n'){
        bool ok;
        peakNum = QString(peakNumChar_3).toInt(&ok,16)*10 + QString(peakNumChar_4).toInt(&ok,16)*1;
    }
    //peakNum是百位数
    else if(peakNumChar_6 == '\n'){
        bool ok;
        peakNum = QString(peakNumChar_3).toInt(&ok,16)*100 + QString(peakNumChar_4).toInt(&ok,16)*10 + QString(peakNumChar_5).toInt(&ok,16)*1;
    }
}

void Demodulation::run()
{
    while(1){

        /*------------遍历CHdataArray[0]即可---------------*/

//        //1. p_echo_net_pack_HEX[] >> demo_CHdata
//        int flag_DEMO = 0;

//        //判断前四位是否是3030，若不是则继续找，若是则从当前位置开始存入一个3030到3030之间的长度，即一个峰值点的数据到demo_CHdata中

//            for(int s=0; s<2044; s++){
//                unsigned char usCHdata1 = udp_recv->p_echo_net_pack_HEX[s];
//                unsigned char usCHdata2 = udp_recv->p_echo_net_pack_HEX[s+1];
//                unsigned char usCHdata3 = udp_recv->p_echo_net_pack_HEX[s+2];
//                unsigned char usCHdata4 = udp_recv->p_echo_net_pack_HEX[s+3];
//                if(usCHdata1=='3' && usCHdata2=='0' && usCHdata3=='3' && usCHdata4=='0'){
//                 flag_DEMO = s;
//                  break;
//                }
//            }

//            int temp = (2048-flag_DEMO)/(peakNum*16);
//            sizeoCHdata = temp*peakNum*16;

//            for(int j=flag_DEMO; j<sizeoCHdata+flag_DEMO; ++j){
//                unsigned char usCHdata = udp_recv->p_echo_net_pack_HEX[j];
//                demo_CHdata[j-flag_DEMO] = usCHdata;
//            }

        if(udp_recv->isStart){

            //1. CHdataArray[0] >> demo_CHdata
            bool isHeadFream = 0;

            //判断前八位是否是3030，若不是则继续找，若是则从当前位置开始存入一个3030到3030之间的长度，即一个峰值点的数据到demo_CHdata中
            do{
    //            if(udp_recv->CHdata2->isEmpty())  msleep(10);
                     unsigned char usCHdata1 = udp_recv->CHdata2->pop();
                     if(usCHdata1 == '3' ){
    //                      if(udp_recv->CHdata2->isEmpty())  msleep(10);
                         unsigned char usCHdata2 = udp_recv->CHdata2->pop();
                         if(usCHdata2 == '0' ){
    //                          if(udp_recv->CHdata2->isEmpty())  msleep(10);
                             unsigned char usCHdata3 = udp_recv->CHdata2->pop();
                             if(usCHdata3 == '3' ){
    //                              if(udp_recv->CHdata2->isEmpty())  msleep(10);
                                 unsigned char usCHdata4 = udp_recv->CHdata2->pop();
                                 if(usCHdata4 == '0' ){

                                                     sizeoCHdata = peakNum*16;

                                                     demo_CHdata[0] = usCHdata1;
                                                     demo_CHdata[1] = usCHdata2;
                                                     demo_CHdata[2] = usCHdata3;
                                                     demo_CHdata[3] = usCHdata4;

                                                     for( int j=4; j<sizeoCHdata; ++j){
    //                                                     if(udp_recv->CHdata2->isEmpty())  msleep(10);
                                                         unsigned char usCHdata = udp_recv->CHdata2->pop();
                                                         demo_CHdata[j] = usCHdata;
                                                     }

                                                     isHeadFream = 1;

                                                     qDebug()<<"Found 3030303030303030!!"<<endl;

                                 }
                             }
                         }
                     }

            }while (isHeadFream==0) ;//isHeadFream == 0时循环，直到不等于0跳出循环

            //2. demo_CHdata[] >> demo_CHdata_DEC_all[]
            int sizeoDemodata = sizeoCHdata;
            for(int m = 0; m<sizeoDemodata; m+=4){
                bool ok;
                int number_DEC = QString(demo_CHdata[m]).toInt(&ok,16)*0 + QString(demo_CHdata[m+1]).toInt(&ok,16)*256 + QString(demo_CHdata[m+2]).toInt(&ok,16)*16 +QString(demo_CHdata[m+3]).toInt(&ok,16)*1;
                if(number_DEC>2047)
                    number_DEC = number_DEC-4096;
                int n = m/4;
                demo_CHdata_DEC_all[n] = number_DEC;
            }

            //3. demo_CHdata_DEC_all[] split into 4 channels
            for(int k = 0; k<sizeoDemodata/4; k+=4){
                int p = k/4;
                demo_CHdata_DEC_1[p] = demo_CHdata_DEC_all[k];
                demo_CHdata_DEC_2[p] = demo_CHdata_DEC_all[k+1];
                demo_CHdata_DEC_3[p] = demo_CHdata_DEC_all[k+2];
                demo_CHdata_DEC_4[p] = demo_CHdata_DEC_all[k+3];
            }

            //4. get Vi Vq
            for(int t = 0; t<sizeoDemodata/16; t++){
                Vi[t] = (float)(demo_CHdata_DEC_1[t] + demo_CHdata_DEC_2[t] - 2 * demo_CHdata_DEC_3[t]);
                Vq[t] = (float)(-sqrt(3) * (demo_CHdata_DEC_1[t] - demo_CHdata_DEC_2[t]));

                /*------------5. Demodulate Phase--------------*/
                Ph[t]=demoduPh(Vi[t],Vq[t]);
    //            float absVi=abs(Vi[t]);
    //            float absVq=abs(Vq[t]);
    //            float z=0,z0=0,dz=0,ph0=0,ph=0;
    //            int numZ0=0;

    //            if(absVi>=absVq){
    //                if(Vi[t]>0){//111 110
    //                    z=-Vq[t]/Vi[t];
    //                    z0=(float) (round(z*100.0)/100.0);//0.01步进的反正切值
    //                    numZ0=(int) (100+z0/0.01);//z0=-1:0.01:1,将其编号为1：1：201
    //                    dz=(z-z0)/(1+(z*z0));
    //                    ph0=dz+atanTable[numZ0];//查表法
    //                    ph=ph0;
    //                }
    //                else if(Vi[t]==0){
    //                    ph=0;
    //                }
    //                else {//vi<0  101 100
    //                    z=-Vq[t]/Vi[t];
    //                    z0=(float) (round(z*100.0)/100.0);//0.01步进的反正切值
    //                    numZ0=(int) (100+z0/0.01);//z0=-1:0.01:1,将其编号为1：1：201
    //                    dz=(z-z0)/(1+(z*z0));
    //                    ph0=dz+atanTable[numZ0];//查表法
    //                    ph= ( ((-Vq[t])>=0) ? 1: -1 ) *M_PI+ph0;
    //                }
    //            }
    //            else {//absVi<absVq
    //                if(Vq[t]<0){//-vq>0
    //                    z=-Vi[t]/Vq[t];
    //                    z0=(float) (round(z*100.0)/100.0);//0.01步进的反正切值
    //                    numZ0=(int) (100+z0/0.01);//z0=-1:0.01:1,将其编号为1：1：201
    //                    dz=(z-z0)/(1+(z*z0));
    //                    ph0=dz+atanTable[numZ0];//查表法
    //                    ph=0.5*M_PI-ph0;
    //                }
    //                else {//-vq<0
    //                    z=-Vi[t]/Vq[t];
    //                    z0=(float) (round(z*100.0)/100.0);//0.01步进的反正切值
    //                    numZ0=(int) (100+z0/0.01);//z0=-1:0.01:1,将其编号为1：1：201
    //                    dz=(z-z0)/(1+(z*z0));
    //                    ph0=dz+atanTable[numZ0];//查表法
    //                    ph=-0.5*M_PI-ph0;
    //                }
    //            }
    //            Ph[t]=ph;

                /*---------------Unwrap Phase---------------*/
                RealPh[t] = Unwrap(Ph[t], t);

    //            if((Ph[t] - PriorPh[t]) > M_PI)//PriorPh[i]时间轴上上一个数据
    //                K[t] = PriorK[t] - 1;//PriorK是K时间轴上上一个数据
    //            else if((Ph[t] - PriorPh[t]) < -M_PI)
    //                K[t] = PriorK[t] + 1;
    //            else
    //                K[t] = PriorK[t];

    //            RealPh[t] = (float)(Ph[t] + 2*K[t]*M_PI);
    //            PriorPh[t] = Ph[t];
    //            PriorK[t] = K[t];

                //6.Ph[] >> DEMOdata_flash ; Ph[] >> DEMOdata_save
                DEMOdata_flash->push(RealPh[t]);
                DEMOdata_save->push(RealPh[t]);
            }
        } // end if
    }//end while

}

//读取反正切值查表文件
void Demodulation::readAtanTable(float *roundNum){
    QString tableFileName;
    tableFileName = QString("C:/Qt_UDP_DAS/atanTable.bin");
    QString path= tableFileName;
    qDebug()<<"atan table file path:"<<path.toStdString().c_str();
    FILE *tableFile=fopen(path.toStdString().c_str(),"rb");
    if(tableFile == NULL)
    {
        char logStr2[64];
        sprintf(logStr2,"Open %s Failed!\n",path.toStdString().c_str());

    }
    fread(roundNum,sizeof(float),201,tableFile);
    fclose(tableFile);
}

//查表法反正切相位解调
float Demodulation::demoduPh(float vi,float vq){
    float absVi=abs(vi);
    float absVq=abs(vq);
    float z=0,z0=0,dz=0,ph0=0,ph=0;
    int numZ0=0;

    if(absVi>=absVq){
        if(vi>0){//111 110
            z=-vq/vi;
            z0=(float) (round(z*100.0)/100.0);//0.01步进的反正切值
            numZ0=(int) (100+z0/0.01);//z0=-1:0.01:1,将其编号为1：1：201
            dz=(z-z0)/(1+(z*z0));
            ph0=dz+atanTable[numZ0];//查表法
            ph=ph0;
        }
        else if(vi==0){
            ph=0;
        }
        else {//vi<0  101 100
            z=-vq/vi;
            z0=(float) (round(z*100.0)/100.0);//0.01步进的反正切值
            numZ0=(int) (100+z0/0.01);//z0=-1:0.01:1,将其编号为1：1：201
            dz=(z-z0)/(1+(z*z0));
            ph0=dz+atanTable[numZ0];//查表法
            ph= ( ((-vq)>=0) ? 1: -1 ) *M_PI+ph0;
        }
    }
    else {//absVi<absVq
        if(vq<0){//-vq>0
            z=-vi/vq;
            z0=(float) (round(z*100.0)/100.0);//0.01步进的反正切值
            numZ0=(int) (100+z0/0.01);//z0=-1:0.01:1,将其编号为1：1：201
            dz=(z-z0)/(1+(z*z0));
            ph0=dz+atanTable[numZ0];//查表法
            ph=0.5*M_PI-ph0;
        }
        else {//-vq<0
            z=-vi/vq;
            z0=(float) (round(z*100.0)/100.0);//0.01步进的反正切值
            numZ0=(int) (100+z0/0.01);//z0=-1:0.01:1,将其编号为1：1：201
            dz=(z-z0)/(1+(z*z0));
            ph0=dz+atanTable[numZ0];//查表法
            ph=-0.5*M_PI-ph0;
        }
    }
    return ph;
}

//相位接缠绕
float Demodulation::Unwrap(float ph, int i)
{
    if((ph - PriorPh[i]) > M_PI)//PriorPh[i]时间轴上上一个数据
        K[i] = PriorK[i] - 1;//PriorK是K时间轴上上一个数据
    else if((ph - PriorPh[i]) < -M_PI)
        K[i] = PriorK[i] + 1;
    else
        K[i] = PriorK[i];

    RealPh[i] = (float)(ph + 2*K[i]*M_PI);
    PriorPh[i] = ph;
    PriorK[i] = K[i];

    return RealPh[i];
}
