#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

//**4、添加附加随机球
#define NUM_SPHERES 50
GLFrame spheres[NUM_SPHERES];

GLShaderManager		shaderManager;			// 着色器管理器
GLMatrixStack		modelViewMatrix;		// 模型视图矩阵
GLMatrixStack		projectionMatrix;		// 投影矩阵
GLFrustum			viewFrustum;			// 视景体
GLGeometryTransform	transformPipeline;		// 几何图形变换管道

GLTriangleBatch		torusBatch;             // 花托批处理
GLBatch				floorBatch;             // 地板批处理

//**2、定义公转球的批处理（公转自转）**
GLTriangleBatch     sphereBatch;            //球批处理

//**3、角色帧 照相机角色帧（全局照相机实例）
GLFrame             cameraFrame;

//**5、添加纹理
//纹理标记数组
GLuint uiTextures[3];

//将TGA加载为纹理
/**
 fileName:纹理文件路径
 minFilter:
 magFilter:
 warpMode:
 */
bool LoadTGATexture(const char *fileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode) {
    
    //指向图像数据的指针
    GLbyte *pBits;
    
    //纹理宽、高、组件
    int nWidth, nHeight, nComponent;
    //纹理格式 tga文件打印出来10进制为6407 对应的16进制为0x1907
    //在glew.h中对应的宏为 #define GL_RGB 0x1907
    GLenum eFormat;
    
    pBits = gltReadTGABits(fileName, &nWidth, &nHeight, &nComponent, &eFormat);
    if (pBits == NULL) {
        return false;
    }
    
    //设置纹理参数
    //wrapMode环绕模式 这里设置为 GL_CLAMP_TO_EDGE
    //x y z 对应 GL_TEXTURE_WRAP_S GL_TEXTURE_WRAP_T GL_TEXTURE_WRAP_R
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    
    //设置过滤方式
    //GL_TEXTURE_MIN_FILTER 和 GL_TEXTURE_MAG_FILTER 为缩小/放大过滤方式
    //过滤方式常用：GL_NEAREST GL_LINEAR  临近过滤 和 线性过滤
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    //载入纹理
    //参数1:纹理维度
    //参数2:level mip贴图层次
    //参数3:纹理内部格式 接受：GL_ALPHA，GL_LUMINANCE，GL_LUMINANCE_ALPHA，GL_RGB，GL_RGBA
    //参数4:宽度
    //参数5:高度
    //参数6:指定边框宽度 必须为0
    //参数7:格式 接受 GL_ALPHA，GL_RGB，GL_RGBA，GL_LUMINANCE，和GL_LUMINANCE_ALPHA
    //参数8:纹理数据类型 接受 GL_UNSIGNED_BYTE，GL_UNSIGNED_SHORT_5_6_5，GL_UNSIGNED_SHORT_4_4_4_4，和GL_UNSIGNED_SHORT_5_5_5_1
    //参数9:指向纹理数据的指针
    glTexImage2D(GL_TEXTURE_2D, 0, nComponent, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);
    
    //释放指针
    free(pBits);
    
    //只有minFilter 等于以下四种模式，才可以生成Mip贴图
    //GL_NEAREST_MIPMAP_NEAREST具有非常好的性能，并且闪烁现象非常弱
    //GL_LINEAR_MIPMAP_NEAREST常常用于对游戏进行加速，它使用了高质量的线性过滤器
    //GL_LINEAR_MIPMAP_LINEAR 和GL_NEAREST_MIPMAP_LINEAR 过滤器在Mip层之间执行了一些额外的插值，以消除他们之间的过滤痕迹。
    //GL_LINEAR_MIPMAP_LINEAR 三线性Mip贴图。纹理过滤的黄金准则，具有最高的精度。
    if(minFilter == GL_LINEAR_MIPMAP_LINEAR ||
       minFilter == GL_LINEAR_MIPMAP_NEAREST ||
       minFilter == GL_NEAREST_MIPMAP_LINEAR ||
       minFilter == GL_NEAREST_MIPMAP_NEAREST)
    //4.纹理生成所有的Mip层
    //参数：GL_TEXTURE_1D、GL_TEXTURE_2D、GL_TEXTURE_3D
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return true;
}


void SetupRC()
{
    //设置清屏颜色到颜色缓冲区
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    //初始化着色器管理器
    shaderManager.InitializeStockShaders();
     
    //开启深度测试和背面剔除
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    //设置大球数据
    gltMakeSphere(torusBatch, 0.4f, 40, 80);
    
    //设置小球数据
    gltMakeSphere(sphereBatch, 0.1f, 26, 13);
    
    //6.设置地板顶点数据&地板纹理 由于使用纹理 只需要确定四个顶点即可
    GLfloat texSize = 10.0f;
    floorBatch.Begin(GL_TRIANGLE_FAN, 4,1);
    floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    floorBatch.Vertex3f(-20.f, -0.41f, 20.0f);
    
    floorBatch.MultiTexCoord2f(0, texSize, 0.0f);
    floorBatch.Vertex3f(20.0f, -0.41f, 20.f);
    
    floorBatch.MultiTexCoord2f(0, texSize, texSize);
    floorBatch.Vertex3f(20.0f, -0.41f, -20.0f);
    
    floorBatch.MultiTexCoord2f(0, 0.0f, texSize);
    floorBatch.Vertex3f(-20.0f, -0.41f, -20.0f);
    floorBatch.End();
    
    //7.随机小球球顶点坐标数据
    for (int i = 0; i < NUM_SPHERES; i++) {
        
        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        
        //对spheres数组中的每一个顶点，设置顶点数据
        spheres[i].SetOrigin(x, 0.0f, z);
    }
    
    //命名纹理对象 需要3个纹理 将数组首地址传进去
    glGenTextures(3, uiTextures);
    
    //分别 绑定纹理对象 到 目标纹理上 设置大小过滤模式 环绕模式
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    LoadTGATexture("Marble.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    LoadTGATexture("Marslike.tga", GL_LINEAR_MIPMAP_LINEAR,
                   GL_LINEAR, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    LoadTGATexture("MoonLike.tga", GL_LINEAR_MIPMAP_LINEAR,
                   GL_LINEAR, GL_CLAMP_TO_EDGE);
    
}

//删除纹理
void ShutdownRC(void)
{
    glDeleteTextures(3, uiTextures);
}

// 屏幕更改大小或已初始化
void ChangeSize(int nWidth, int nHeight)
{
    //1.设置视口
    glViewport(0, 0, nWidth, nHeight);
    
    //2.设置投影方式
    viewFrustum.SetPerspective(35.0f, float(nWidth)/float(nHeight), 1.0f, 100.0f);
    
    //3.将投影矩阵加载到投影矩阵堆栈,
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    modelViewMatrix.LoadIdentity();
    
    //4.将投影矩阵堆栈和模型视图矩阵对象设置到管道中
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
   
    
}

void drawSomething(GLfloat yRot)
{
    //1.定义光源位置&漫反射颜色
    static GLfloat vWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    static GLfloat vLightPos[] = { 0.0f, 3.0f, 0.0f, 1.0f };
    
    //绘制小球
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    for (int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                     modelViewMatrix.GetMatrix(),
                                     transformPipeline.GetProjectionMatrix(),
                                     vLightPos,
                                     vWhite,
                                     0);
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
    }
    
    //绘制大球
    //先平移下
    modelViewMatrix.Translate(0.0f, 0.2f, -2.5f);
    //由于要旋转 所以入栈
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
    //绑定纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    torusBatch.Draw();
    modelViewMatrix.PopMatrix();
    
    //绘制公转的小球
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot * -2.0f, 0.0f, 1.0f, 0.0f);
    modelViewMatrix.Translate(0.8f, 0.0f, 0.0f);
    //绑定纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    sphereBatch.Draw();
    modelViewMatrix.PopMatrix();
}



//进行调用以绘制场景
void RenderScene(void)
{
    //清除颜色缓冲区 深度缓冲区 模型缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    //地板颜色
    static GLfloat vFloorColor[] = { 1.0f, 1.0f, 0.0f, 0.75f};
    
    //根据定时器 旋转
    static CStopWatch rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60;
    
    //1⃣️压栈 单元矩阵
    modelViewMatrix.PushMatrix();
    
    //设置观察者矩阵
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.MultMatrix(mCamera);
    
    //2⃣️压栈 镜面
    modelViewMatrix.PushMatrix();
    
    //沿y轴翻转
    modelViewMatrix.Scale(1.0f, -1.0f, 1.0f);
    //为了更加形象 沿y轴平移一定距离
    modelViewMatrix.Translate(0.0f, 0.8f, 0.0f);
    
    //由于镜面效果 指定顺时针为正面
    glFrontFace(GL_CW);
    
    //绘制地面下 的镜面部分
    drawSomething(yRot);
    
    //绘制完镜面 将正背面规定 恢复
    glFrontFace(GL_CCW);
    
    //对应2⃣️出栈
    modelViewMatrix.PopMatrix();
    
    //绘制地板 由于地板没有变化 不用压栈
    //开启混合
    glEnable(GL_BLEND);
    //指定混合方式
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //绑定地板纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    
    /**
     纹理调整着色器(将一个基本色乘以一个取自纹理的单元nTextureUnit的纹理)
     参数1：GLT_SHADER_TEXTURE_MODULATE
     参数2：模型视图投影矩阵
     参数3：颜色
     参数4：纹理单元（第0层的纹理单元）
     */
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE,
                                 transformPipeline.GetModelViewProjectionMatrix(),
                                 vFloorColor,
                                 0);
    
    //开始绘制地板
    floorBatch.Draw();

    //关闭混合
    glDisable(GL_BLEND);
    
    //绘制地上部分 由于是最后绘制 可以不用压栈,也可以对应的压栈/出栈 且由于镜面部分的模型矩阵已经出栈 翻转效果已经去掉了
    drawSomething(yRot);
    
    //对应1⃣️出栈
    modelViewMatrix.PopMatrix();
    
    //交换缓冲区
    glutSwapBuffers();
    
    //重新提交渲染
    glutPostRedisplay();
}


//**3.移动照相机参考帧，来对方向键作出响应
void SpeacialKeys(int key,int x,int y)
{
    
    float linear = 0.1f;
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        
        //MoveForward 平移
        cameraFrame.MoveForward(linear);
    }
    
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    
    if (key == GLUT_KEY_LEFT) {
        //RotateWorld 旋转
        cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
    }
    
    
    
}


int main(int argc, char* argv[])
{
    gltSetWorkingDirectory(argv[0]);
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800,600);
    
    glutCreateWindow("OpenGL SphereWorld");
    
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    glutSpecialFunc(SpeacialKeys);
    
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    
    SetupRC();
    glutMainLoop();
    ShutdownRC();
    return 0;
}
