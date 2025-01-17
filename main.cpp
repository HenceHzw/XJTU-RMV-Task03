#include "windmill.hpp"
#include <ceres/ceres.h>
#include <ceres/solver.h>

using namespace std;
using namespace cv;

const double pi = 3.1415926;
double A , omega , phi, b, c;
// 定义残差结构体
struct AngleSpeedResidual {
    AngleSpeedResidual(double t, double observed)
        : t_(t), observed_(observed) {}

    // 残差计算
    template <typename T>
    bool operator()(const T* const A, const T* const omega, const T* const phi, const T* const b,const T* const c, T* residual) const {
       
        // 计算拟合值
        residual[0] = observed_ - ((A[0]/omega[0]) * cos(omega[0] * t_ + phi[0]) + b[0]*t_ + c[0]);
        return true;
    }

 private:
    const double t_;
    const double observed_;
};

void fitAngleSpeed(const std::vector<double>& times, const std::vector<double>& speeds, double &A, double &omega, double &phi, double &b, double &c) {
    // 构造问题
    ceres::Problem problem;
    for (size_t i = 0; i < times.size(); ++i) {
        // 为每个数据点添加残差块
       
        problem.AddResidualBlock(
            new ceres::AutoDiffCostFunction<AngleSpeedResidual, 1,1,1,1,1,1>(
                new AngleSpeedResidual(times[i], speeds[i])),
             nullptr, &A ,&omega, &phi, &b, &c);
    }
    // new ceres::CauchyLoss(0.5) 

    // 配置求解器
    ceres::Solver::Options options;
    options.logging_type=ceres::SILENT; 
    options.linear_solver_type = ceres::DENSE_SCHUR;
    options.minimizer_progress_to_stdout = true;
    // options.max_num_iterations = 1000; // 增加最大迭代次数
    // options.function_tolerance = 1e-6;  // 更严格的收敛标准
    // options.max_num_iterations = 2000;
    // options.function_tolerance = 1e-9;
    // options.gradient_tolerance = 1e-10;


    // 求解
    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);

    // 输出拟合结果
    // std::cout << "Estimated A: " << abs(A) << "\n";
    // std::cout << "Estimated ω: " << abs(omega) << "\n";
    // std::cout << "Estimated φ: " << abs(phi) << "\n";
    // std::cout << "Estimated b: " << abs(b) << "\n";
    // std::cout << "Estimated c: " << abs(c) << "\n";
    // cout<<endl<<endl;
}


int main()
{
    double avg_time=0;int fail_num=0;
    for(int num=0;num<10;num++)
    {
        // 开始计时
        auto start = std::chrono::high_resolution_clock::now();


        std::chrono::milliseconds t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        double time_Begin=(double)t.count();
        WINDMILL::WindMill wm(time_Begin);
        cv::Mat src;


        vector<double>angles;
        vector<double> Angle_speeds;
        vector<double> Angle_Times;
        int count = 0;
        double old_angle=-99999;
        double old_time=0;
        // 初始化拟合参数: A, ω, φ, b, c
        A =1.785, omega = 2.884, phi=0.81, b= 0.305, c=0.1;
        int flag =0;

        while (1)
        {
            count++;
            t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            double time_now=(double)t.count();
            src = wm.getMat(time_now);

            double time= (time_now- time_Begin)/1000;
            
            //==========================代码区========================//

            // 1. 将图像转换为 HSV 颜色空间，以便提取红色区域
            cv::Mat hsvImg;
            cv::cvtColor(src, hsvImg, cv::COLOR_BGR2HSV);
            
            // 2. 定义红色的 HSV 范围
            cv::Mat mask1, mask2;
            cv::inRange(hsvImg, cv::Scalar(0, 100, 100), cv::Scalar(10, 255, 255), mask1);  // 红色范围1
            cv::inRange(hsvImg, cv::Scalar(160, 100, 100), cv::Scalar(180, 255, 255), mask2); // 红色范围2
            
            // 3. 合并两个红色区域的掩码
            cv::Mat redMask = mask1 | mask2;
            
            // 4. 使用形态学操作去除噪声
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
            cv::morphologyEx(redMask, redMask, cv::MORPH_OPEN, kernel);
            
            // 5. 查找红色区域的轮廓
            vector<vector<Point>> contours;
            vector<Vec4i> hierarchy;
            cv::findContours(redMask, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
            
            // 6. 绘制检测到的轮廓
            float minarea=999999,maxarea=-999999;
            int flag_R=0, flag_fan=0;


            for (size_t i = 0; i < contours.size(); i++)//   寻找  R
            {
                double area = cv::contourArea(contours[i]);
                if(hierarchy[i][3]==-1&&area<=minarea)
                {
                    flag_R=i;
                    minarea=area;
                }
                if(area>=maxarea)//顺便求最大area 
                {
                    maxarea=area;
                }
            }
            for (size_t i = 0; i < contours.size(); i++)//   寻找  fan
            {
                double area = cv::contourArea(contours[i]);
                if(hierarchy[i][3]==-1&&area>minarea&&area<maxarea-1000)
                {
                    cv::drawContours(src, contours, (int)i, cv::Scalar(255, 255, 255), 2);  
                    flag_fan=hierarchy[i][2];
                }
            }
            cv::drawContours(src, contours, (int)flag_fan, cv::Scalar(255, 255, 255), 2);  // 用白色绘制轮廓
            cv::drawContours(src, contours, (int)flag_R, cv::Scalar(255, 255, 255), 2);
            cv::Point center_R(0, 0),center_fan(0,0);    //求 R 和 fan 的中心点
            for(int i=0;i<contours[flag_R].size();i++)
            {
                center_R+=contours[flag_R][i];
            }
            for(int i=0;i<contours[flag_fan].size();i++)
            {
                center_fan+=contours[flag_fan][i];
            }
            center_R.x/=contours[flag_R].size();
            center_fan.x/=contours[flag_fan].size();
            center_R.y/=contours[flag_R].size();
            center_fan.y/=contours[flag_fan].size();
            circle(src,Point(center_fan.x,center_fan.y),3,cv::Scalar(0,255,0),-1);
            circle(src,Point(center_R.x,center_R.y),3,cv::Scalar(0,255,0),-1);

            double angle = -atan2(center_fan.y - center_R.y, center_fan.x - center_R.x);
            
            // if(count>=4)                //平滑处理
            // {
            //     for(int i=0;i<3;i++)
            //     {
            //         angle+=angles[count-i-2];
            //     }
            //     angle/=4;
            // }
            
            angles.push_back(angle);
            Angle_Times.push_back(time);

            // if(old_angle<-10)       //获取角速度
            // {
            //     old_angle = angle;
            //     old_time = time;
            //     continue;
            // }
            // if(old_angle>0&&angle<0)
            // {
            //     old_angle-=2*pi;
            // }

            // double angle_speed = (angle -old_angle)/(time - old_time);
            
            // Angle_speeds.push_back(angle_speed);
            // Angle_Times.push_back((time+old_time)/2);
            // old_angle = angle;
            // old_time = time;


            if(count%250==0)                 //每隔250帧拟合一次
            {
                fitAngleSpeed(Angle_Times, angles, A, omega ,phi, b, c);
                
                // // 清空数据，重新积累数据点
                // Angle_speeds.clear();
                // Angle_Times.clear();
            }

            
            
            // 计算 R 和风车扇叶之间的中点
            cv::Point midPoint = (center_fan + center_R) / 2;

            // 8. 在中点处绘制一个圆形
            cv::circle(src, midPoint, 20, cv::Scalar(0, 255, 0), 3);  // 绿色圆形


            double real_A=0.785,real_omega=1.884,real_phi=1.81,real_b=1.305;      //收敛判断
            double res_A=abs(abs(A)-real_A),res_omega=abs(abs(omega)-real_omega),res_phi=abs(abs(phi)-real_phi),res_b=abs(abs(b)-real_b);
            if(res_A<=real_A*0.05&&res_omega<=real_omega*0.05&&res_b<=real_b*0.05&&res_phi<=real_phi*0.05)
            {
                break;
            }

            if(count==2000)
            {
                cout<<"第 "<<num+1<<" 次收敛超时******************收敛失败!  重新拟合"<<endl;
                fail_num+=1;
                flag=1;
                num-=1;
                break;
            }


            

            imshow("windmill", src);
            //=======================================================//
            
            
            if (waitKey(1) == 27)  // 按下 'Esc' 键退出
            {
                break;
            }
        }
        if(flag == 0)
        {
            cout<<"第 "<<num+1<<" 次收敛结果： "<<endl;
            std::cout << "Estimated A: " << abs(A) << "\n";
            std::cout << "Estimated ω: " << abs(omega) << "\n";
            std::cout << "Estimated φ: " << abs(phi) << "\n";
            std::cout << "Estimated b: " << abs(b) << "\n";
            // std::cout << "Estimated c: " << abs(c) << "\n";
            cout<<endl<<endl;

        }

        // 结束计时
        auto end = std::chrono::high_resolution_clock::now();
        
        // 计算持续时间并转换为毫秒
        std::chrono::duration<double, std::milli> duration = end - start;

        // 输出运行时间（精确到毫秒）
        std::cout<<"程序运行时间: " << duration.count() << " 毫秒\n";

        avg_time+=duration.count();     //失败拟合的时间也算在总时间里面
           
    }


    
    avg_time/=10;
    std::cout << "程序平均运行时间: " << avg_time << " 毫秒\n";
    // cout<<"100次拟合中，共有 "<<fail_num<<" 次拟合失败";
    
   
    return 0;
} 