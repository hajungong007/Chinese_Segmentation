#include "dialog.h"
#include "ui_dialog.h"
#include <QButtonGroup>
#include<QMessageBox>
#include <QDebug>
#include <QTextStream>
#include <stdio.h>
#include <QTextCodec>
#include<QFileDialog>

#include<sstream>
#include<iostream>
#include<string>
#include<windows.h>
#include<fstream>
#include<set>
#include<map>
using namespace std;

int batchn=0;

string inputfile;
string outputdir;

int chooseparameter=-1;
int algorithm=-1;

QString str2qstr(const string str)
{
    return QString::fromLocal8Bit(str.data());
}

//给定HMM模型的三个参数
map<char, double>initial;  //初始矩阵
double convert[5][5];   //转移矩阵
map <string,double> emit1[4];  //发射矩阵

map<char, int> t;

double bestattime[10000][4];  //viterbi算法的δ_t (i)
int previousnode[10000][4];   //viterbi算法的ψ_t(i)

//监督学习

wstring sentence[200000]; //训练集中的句子
wstring processedsentence[200000];//处理后的句子，以非汉字,空格，顿号，引号的符号分割，和标签一一对应
string flag[200000];  //训练集中句子后的标签

//监督学习HMM模型的三个参数
double supervisedinitial[4];//初始矩阵
double supervisedconvert[4][4];//转移矩阵
map<string, double> supervisedemit[4];//发射矩阵


int flagint[200000][700];//转化为整数的flag
int ntotal[4]; //四个隐状态分别出现的次数
int ntotalsupervisedconvert[4];//四个隐状态分别转移的次数

char int2char[4] = { 'B','M','E','S' };
map<char, int> char2int;


//用于计算前向后向概率
double alpha[1000][4];
double beta[1000][4];

double forwardp;//由前向算法计算得到的观测序列出现概率
double backward;//由后向算法计算得到的观测序列出现概率(两者数值相同)
double supervisedforwardp;
double supervisedbackward;
double expectation[1000][4];//近似算法的γ_t(i)


BOOL StringToWString(const std::string &str, std::wstring &wstr)
{
          int nLen = (int)str.length();
          wstr.resize(nLen, L' ');

           int nResult = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), nLen, (LPWSTR)wstr.c_str(), nLen);

              if (nResult == 0)
              {
                  return FALSE;
              }

          return TRUE;
}

std::string WChar2Ansi(LPCWSTR pwszSrc)
{
    int nLen = WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, NULL, 0, NULL, NULL);

    if (nLen <= 0) return std::string("");

    char* pszDst = new char[nLen];
    if (NULL == pszDst) return std::string("");

    WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, pszDst, nLen, NULL, NULL);
    pszDst[nLen - 1] = 0;

    std::string strTemp(pszDst);
    delete[] pszDst;

    return strTemp;
}

void  initialize()
{

    char2int['B'] = 0;
    char2int['M'] = 1;
    char2int['E'] = 2;
    char2int['S'] = 3;

}

//获取给定的HMM模型参数
void gettheparameter()
{
    t['B'] = 0;
    t['M'] = 1;
    t['E'] = 2;
    t['S'] = 3;
    initial[0] = 0.6887918653263693;
    initial[2] = 0;
    initial[3] = 0.31120813467363073;
    initial[1] = 0;
    convert[0][2] = 0.8623367940544834;
    convert[0][1] = 0.13766320594551662;
    convert[1][2] = 0.7024280846522946;
    convert[1][1] = 0.2975719153477054;
    convert[2][0] = 0.5544856664818801;
    convert[2][3] = 0.4455143335181199;
    convert[3][0] = 0.48617131037009215;
    convert[3][3] = 0.5138286896299078;
    ifstream dic("matrix.txt");
    if (!dic)
    {
        cout << "no such file" << endl;
    }
    string word;
    int current = 0;
    int n = 0;
    while (getline(dic, word))
    {
        string chineseword = word.substr(7, 2);
        if (n == 0) current = 0;
        else if (n == 5051) current = 2;
        else if (n == 10356) current = 1;
        else if (n == 14985) current = 3;
        n++;
        emit1[current][chineseword] = atof(word.substr(12).c_str());
    }
}

// 在程序开始运行时执行一次，读取给定参数及通过pku_training监督学习得到另一组参数
void umain()
{
    QTextStream cin(stdin, QIODevice::ReadOnly);

    QTextStream cout(stdout, QIODevice::WriteOnly);
    gettheparameter();
    initialize();
    wcout.imbue(locale(locale(), "", LC_CTYPE));

        ifstream inputfile("pku_training.txt");
        if (!inputfile)
        {
            cout<<"no such file"<<endl;
        }

        string line;
        int nsentence = 0;
        wstring temp = L" ";
        while (getline(inputfile, line))
        {
            wstring thisline;
            string tline = line;
            bool pan = StringToWString(line, thisline);
            if (pan)
            {
                for (int i = 0; i < thisline.length(); i++)
                {
                    if ((thisline[i] >= 48 && thisline[i] <= 57) || (thisline[i] >= 65296 && thisline[i] <= 65305)) //阿拉伯数字转化成汉字
                    {
                        switch (thisline[i])
                        {
                        case 48:
                        case 65296:
                            thisline[i] = L'零';
                            break;
                        case 49:
                        case 65297:
                            thisline[i] = L'一';
                            break;
                        case 50:
                        case 65298:
                            thisline[i] = L'二';
                            break;
                        case 51:
                        case 65299:
                            thisline[i] = L'三';
                            break;
                        case 52:
                        case 65300:
                            thisline[i] = L'四';
                            break;
                        case 53:
                        case 65301:
                            thisline[i] = L'五';
                            break;
                        case 54:
                        case 65302:
                            thisline[i] = L'六';
                            break;
                        case 55:
                        case 65303:
                            thisline[i] = L'七';
                            break;
                        case 56:
                        case 65304:
                            thisline[i] = L'八';
                            break;
                        case 57:
                        case 65305:
                            thisline[i] = L'九';
                            break;
                        default:
                            break;
                        }

                    }
                    if (thisline[i] == L'、' || thisline[i] == L'“' || thisline[i] == L'”'||thisline[i]==L'《'||thisline[i]==L'》')
                    {
                        thisline[i] = ' ';
                    }
                    if (thisline[i] >= 0x4e00 && thisline[i] <= 0x9fbb || thisline[i] == ' ')  //分割句子
                        temp += thisline[i];
                    else
                    {
                        temp += ' ';
                        sentence[nsentence] = temp;
                        nsentence++;
                        temp = L" ";
                    }
                }
                temp += ' ';
                sentence[nsentence] = temp;
                nsentence++;
                temp = L" ";
            }
        }
        long int nreal = -1;
        for (int sentenceno = 0; sentenceno < nsentence; sentenceno++)
        {
            wstring thissentence = sentence[sentenceno];
            int pan = 0;
            for (int i = 0; i < thissentence.length(); i++)
            {
                if (thissentence[i] == ' ')
                    continue;
                if (!pan)
                {
                    nreal++;
                }
                pan = 1;
                if (thissentence[i - 1] == ' '&&thissentence[i + 1] == ' ')
                {
                    processedsentence[nreal] += thissentence[i];
                    flag[nreal] += 'S';
                }
                else if (thissentence[i - 1] == ' '&&thissentence[i + 1] != ' ')
                {
                    processedsentence[nreal] += thissentence[i];
                    flag[nreal] += 'B';
                }
                else if (thissentence[i - 1] != ' '&&thissentence[i + 1] == ' ')
                {
                    processedsentence[nreal] += thissentence[i];
                    flag[nreal] += 'E';
                }
                else if (thissentence[i - 1] != ' '&&thissentence[i + 1] != ' ')
                {
                    processedsentence[nreal] += thissentence[i];
                    flag[nreal] += 'M';
                }
                else
                {
                    cout << sentenceno << endl;
                    cout << "error" << endl;
                }
            }
        }
        for (int i = 0; i <= nreal; i++)
        {
            for (int j = 0; j < flag[i].length(); j++)
            {
                flagint[i][j] = char2int[flag[i][j]];
            }
        }
        int ntotalbegin = 0;
        for (int i = 0; i <= nreal; i++)
        {
            int ppan = 0;
            supervisedinitial[flagint[i][0]]++;                //统计监督学习各个隐状态初始出现的概率
            ntotalbegin++;                                     //总句子数
            for (int j = 0; j < processedsentence[i].length() - 1; j++)
            {
                supervisedconvert[flagint[i][j]][flagint[i][j + 1]]++;   //监督学习时状态转移的次数
                ntotalsupervisedconvert[flagint[i][j]]++;                //4个隐状态总转移数
            }
        }
        for (int i = 0; i < 4; i++)
        {
            supervisedinitial[i] = supervisedinitial[i] / ntotalbegin;
        }
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                supervisedconvert[i][j] /= ntotalsupervisedconvert[i];
            }
        }
        for (int i = 0; i <= nreal; i++)
        {
            for (int j = 0; j < processedsentence[i].length(); j++)
            {
                wstring twstring = processedsentence[i].substr(j, 1);      //取每一个汉字
                string thisword = WChar2Ansi(&twstring[0]);
                ntotal[flagint[i][j]]++;                                  //统计每个隐状态出现的次数
                supervisedemit[flagint[i][j]][thisword]++;                //监督学习时的发射矩阵
            }
        }
        for (int i = 0; i < 4; i++)
        {
            for (int test = 0x4e00; test < 0x9fbb; test++)
            {
                wchar_t testw = test;
                string thischar = WChar2Ansi(&testw);
                if (supervisedemit[i][thischar] != 0)
                    supervisedemit[i][thischar] /= ntotal[i];

            }

        }
    return ;
}

//使用给定参数进行分词
string  fenci(wstring a )
{
    int T = a.length();
    for (int t = 0; t< T; t++)
    {
        wstring twstring = a.substr(t, 1);
        string thisword = WChar2Ansi(&twstring[0]);
        for (int i=0;i<4;i++)
        {
            if (emit1[i][thisword]==0)
                emit1[i][thisword]=1e-10;
        }
        if (t == 0)
        {
            for (int i = 0; i < 4; i++)
            {

                bestattime[0][i] = initial[i] * emit1[i][thisword];
                previousnode[0][i] = 0;
            }
        }
        else
        {
            for (int i = 0; i< 4; i++)
            {
                double tmax = 0;
                int tmaxno = 0;
                for (int previousi = 0; previousi < 4; previousi++)
                {
                    if (bestattime[t - 1][previousi] * convert[previousi][i] > tmax)
                    {
                        tmax = bestattime[t - 1][previousi] * convert[previousi][i];
                        tmaxno = previousi;
                    }
                }
                bestattime[t][i] = tmax*emit1[i][thisword];
                previousnode[t][i] = tmaxno;
            }
        }
    }
    double tmax = 0;
    double tmaxno;
    string result;
    for (int i = 0; i < 4; i++)
    {
        if (bestattime[T - 1][i] > tmax)
        {
            tmax = bestattime[T - 1][i];
            tmaxno = i;
        }
    }
    int it = tmaxno;
    result += int2char[it];
    for (int t = T - 1; t >= 1; t--)
    {
        it = previousnode[t][it];
        result += int2char[it];
    }
    reverse(result.begin(), result.end());
    string final;
    for (int i = 0; i < result.length(); i++)
    {
        wstring twstring = a.substr(i, 1);
        string thisword = WChar2Ansi(&twstring[0]);
        final+=thisword;
        if (result[i] == 'S' || result[i] == 'E')
        {
            final+="/";
        }
    }
    return final;
}

//使用监督学习得到的参数进行分词
string  supervisedfenci(wstring a)
{
    int T = a.length();
    for (int t = 0; t< T; t++)
    {
        wstring twstring = a.substr(t, 1);
        string thisword = WChar2Ansi(&twstring[0]);
        for (int i=0;i<4;i++)
        {
            if (supervisedemit[i][thisword]==0)
               supervisedemit[i][thisword]=1e-10;
        }
        if (t == 0)
        {
            for (int i = 0; i < 4; i++)
            {

                bestattime[0][i] = supervisedinitial[i] * supervisedemit[i][thisword];
                previousnode[0][i] = 0;
            }
        }
        else
        {
            for (int i = 0; i< 4; i++)
            {
                double tmax = 0;
                int tmaxno = 0;
                for (int previousi = 0; previousi < 4; previousi++)
                {
                    if (bestattime[t - 1][previousi] * supervisedconvert[previousi][i] > tmax)
                    {
                        tmax = bestattime[t - 1][previousi] * supervisedconvert[previousi][i];
                        tmaxno = previousi;
                    }
                }
                bestattime[t][i] = tmax*supervisedemit[i][thisword];
                previousnode[t][i] = tmaxno;
            }
        }
    }
    double tmax = 0;
    double tmaxno;
    string result;
    for (int i = 0; i < 4; i++)
    {
        if (bestattime[T - 1][i] > tmax)
        {
            tmax = bestattime[T - 1][i];
            tmaxno = i;
        }
    }
    int it = tmaxno;
    result += int2char[it];
    for (int t = T - 1; t >= 1; t--)
    {
        it = previousnode[t][it];
        result += int2char[it];
    }
    reverse(result.begin(), result.end());
    string final;
    for (int i = 0; i < result.length(); i++)
    {
        wstring twstring = a.substr(i, 1);
        string thisword = WChar2Ansi(&twstring[0]);
        final+=thisword;
        if (result[i] == 'S' || result[i] == 'E')
        {
            final+="/";
        }
    }
    return final;
}

//计算前向概率
double calculateforward(wstring a)
{
    memset(alpha, 0, sizeof(alpha));
    int T = a.length();
    for (int t = 0; t < T; t++)
    {
        wstring twstring = a.substr(t, 1);
        string thisword = WChar2Ansi(&twstring[0]);
        for (int i=0;i<4;i++)
        {
            if (emit1[i][thisword]==0)
                emit1[i][thisword]=1e-10;
        }
        if (t == 0)
        {
            for (int i = 0; i < 4; i++)
            {

                alpha[0][i] = initial[i] * emit1[i][thisword];
            }
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                for (int previousi = 0; previousi < 4; previousi++)
                {
                    alpha[t][i] += alpha[t - 1][previousi] * convert[previousi][i];
                }
                alpha[t][i] *= emit1[i][thisword];
            }
        }
    }
    double resultprob = 0;
    for (int i = 0; i < 4; i++)
        resultprob += alpha[T - 1][i];
    return resultprob;
}

//计算前向概率（监督学习）
double supervisedcalculateforward(wstring a)
{
    memset(alpha, 0, sizeof(alpha));
    int T = a.length();
    for (int t = 0; t < T; t++)
    {
        wstring twstring = a.substr(t, 1);
        string thisword = WChar2Ansi(&twstring[0]);
        for (int i=0;i<4;i++)
        {
            if (supervisedemit[i][thisword]==0)
                supervisedemit[i][thisword]=1e-10;
        }
        if (t == 0)
        {
            for (int i = 0; i < 4; i++)
            {

                alpha[0][i] = supervisedinitial[i] * supervisedemit[i][thisword];
            }
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                for (int previousi = 0; previousi < 4; previousi++)
                {
                    alpha[t][i] += alpha[t - 1][previousi] * supervisedconvert[previousi][i];
                }
                alpha[t][i] *= supervisedemit[i][thisword];
            }
        }
    }
    double resultprob = 0;
    for (int i = 0; i < 4; i++)
        resultprob += alpha[T - 1][i];
    return resultprob;
}

//计算后向概率
double calculatebackward(wstring a)
{
    memset(beta, 0, sizeof(beta));
    int T = a.length();
    for (int t = T - 1; t >= 0; t--)
    {
        if (t == T - 1)
        {
            for (int i = 0; i < 4; i++)
            {
                beta[T - 1][i] = 1;
            }
        }
        else
        {
            wstring twstring = a.substr(t + 1, 1);
            string nextword = WChar2Ansi(&twstring[0]);
            for (int i=0;i<4;i++)
            {
                if (emit1[i][nextword]==0)
                    emit1[i][nextword]=1e-10;
            }
            for (int i = 0; i < 4; i++)
            {
                for (int nexti = 0; nexti < 4; nexti++)
                {
                    beta[t][i] += convert[i][nexti] * beta[t + 1][nexti] * emit1[nexti][nextword];
                }
            }
        }
    }
    double resultprob = 0;
    wstring twstring = a.substr(0, 1);
    string firstword = WChar2Ansi(&twstring[0]);
    for (int i = 0; i < 4; i++)
    {
        resultprob += initial[i] * beta[0][i] * emit1[i][firstword];
    }
    return resultprob;
}

//计算后向概率(监督学习)
double supervisedcalculatebackward(wstring a)
{
    memset(beta, 0, sizeof(beta));
    int T = a.length();
    for (int t = T - 1; t >= 0; t--)
    {
        if (t == T - 1)
        {
            for (int i = 0; i < 4; i++)
            {
                beta[T - 1][i] = 1;
            }
        }
        else
        {
            wstring twstring = a.substr(t + 1, 1);
            string nextword = WChar2Ansi(&twstring[0]);
            for (int i=0;i<4;i++)
            {
                if (supervisedemit[i][nextword]==0)
                    supervisedemit[i][nextword]=1e-10;
            }
            for (int i = 0; i < 4; i++)
            {
                for (int nexti = 0; nexti < 4; nexti++)
                {
                    beta[t][i] += supervisedconvert[i][nexti] * beta[t + 1][nexti] * supervisedemit[nexti][nextword];
                }
            }
        }
    }
    double resultprob = 0;
    wstring twstring = a.substr(0, 1);
    string firstword = WChar2Ansi(&twstring[0]);
    for (int i = 0; i < 4; i++)
    {
        resultprob += supervisedinitial[i] * beta[0][i] * supervisedemit[i][firstword];
    }
    return resultprob;
}

//近似算法
string approximation(wstring a)
{
    int T = a.length();
    for (int t = 0; t < T; t++)
    {
        for (int i = 0; i < 4; i++)
        {
            expectation[t][i] = alpha[t][i] * beta[t][i];
            expectation[t][i] = expectation[t][i] / forwardp;
        }
    }
    int tresult[1000];
    for (int t = 0; t < T; t++)
    {
        double tmax = 0;
        int tno = 0;
        for (int i = 0; i < 4; i++)
        {
            if (expectation[t][i] > tmax)
            {
                tmax = expectation[t][i];
                tno = i;
            }

        }
        tresult[t] = tno;
    }
    string final;
    for (int i = 0; i < T; i++)
    {
        wstring twstring = a.substr(i, 1);
        string thisword = WChar2Ansi(&twstring[0]);
        final+=thisword;
        if (int2char[tresult[i]] == 'S' || int2char[tresult[i]] == 'E')
          final+="/";
    }
   return final;
}

//近似算法(监督学习)
string supervisedapproximation(wstring a)
{
    int T = a.length();
    for (int t = 0; t < T; t++)
    {
        for (int i = 0; i < 4; i++)
        {
            expectation[t][i] = alpha[t][i] * beta[t][i];
            expectation[t][i] = expectation[t][i] / supervisedforwardp;
        }
    }
    int tresult[1000];
    for (int t = 0; t < T; t++)
    {
        double tmax = 0;
        int tno = 0;
        for (int i = 0; i < 4; i++)
        {
            if (expectation[t][i] > tmax)
            {
                tmax = expectation[t][i];
                tno = i;
            }

        }
        tresult[t] = tno;
    }
    string final;
    for (int i = 0; i < T; i++)
    {
        wstring twstring = a.substr(i, 1);
        string thisword = WChar2Ansi(&twstring[0]);
        final+=thisword;
        if (int2char[tresult[i]] == 'S' || int2char[tresult[i]] == 'E')
          final+="/";
    }
   return final;
}

string getanswer(wstring a2)
{
    string test;
    if (chooseparameter==0)
       {
           if (algorithm==0)
               test=fenci(a2);
           else if (algorithm==1)
           {
               forwardp=calculateforward(a2);
               backward=calculatebackward(a2);
               test=approximation(a2);
           }
       }
        else if (chooseparameter==1)
      {
         if (algorithm==0)
           test=supervisedfenci(a2);
         else if (algorithm==1)
         {
             supervisedforwardp=supervisedcalculateforward(a2);
             supervisedbackward=supervisedcalculatebackward(a2);
             test=supervisedapproximation(a2);

         }
      }
    return test;
}

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    umain();
}




Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pushButton_clicked()
{

    if (algorithm==-1||chooseparameter==-1)
    {
          QMessageBox::information(NULL, u8"提示", u8"请选择模型参数及算法", QMessageBox::Yes , QMessageBox::Yes);
          return ;
    }

   QString a1=ui->textEdit->toPlainText();
   wstring a2=a1.toStdWString();
   string  test;
    int l=0;
    wstring temp=L"";
    while (l<a2.length())
    {
        if (a2[l] == L'、' || a2[l] == L'“' || a2[l] == L'”'||a2[l]==L'《'||a2[l]==L'》')
        {
            a2[l] = ' ';
        }
        if (a2[l] >= 0x4e00 && a2[l] <= 0x9fbb || a2[l] == ' ')
        {
            if (a2[l] != ' ')
                temp += a2[l];
        }
        else
         {
            if (temp.length())
            {
               test=test+getanswer(temp)+'\n';
            }
            temp = L"";
        }
        l++;
    }
    if (temp.length())
    {
          test=test+getanswer(temp)+'\n';
    }

    QString test1=str2qstr(test);
    ui->output->setPlainText(test1);
}

void Dialog::on_parameter1_clicked()
{
   chooseparameter=0;
}

void Dialog::on_parameter2_clicked()
{
   chooseparameter=1;
}

void Dialog::on_algorithm1_clicked()
{
    algorithm=0;
}

void Dialog::on_algorithm2_clicked()
{
    algorithm=1;
}

void Dialog::on_inputfile_clicked()
{
     QString fileName=QFileDialog::getOpenFileName(this,"open this document",QDir::currentPath(),"allfiles(*.*)");
     inputfile=fileName.toStdString();
     ui->labelforinput->setText(fileName);
}

void Dialog::on_pushButton_3_clicked()
{
      QString file_path = QFileDialog::getExistingDirectory(this,"请选择模板保存路径...","./");
      outputdir=file_path.toStdString();
      ui->labelforoutput->setText(file_path);
}

void Dialog::on_batch_clicked()
{

    if (!outputdir.length()||!inputfile.length())
     {
        QMessageBox::information(NULL, u8"提示", u8"请选择对应文件及路径", QMessageBox::Yes , QMessageBox::Yes);
        return ;
     }

    if (algorithm==-1||chooseparameter==-1)
    {
          QMessageBox::information(NULL, u8"提示", u8"请选择模型参数及算法", QMessageBox::Yes , QMessageBox::Yes);
          return ;
    }

    ifstream testfile(inputfile);
    if (!testfile)
        cout << "test file not exist!" << endl;
    string num;
    stringstream stream;
    stream<<batchn;
    batchn++;
    num=stream.str();
    string thisoutputdir=outputdir+"/result"+num+".txt";
    ofstream testoutput(thisoutputdir);
    wstring temp = L"";
    string line;
    while (getline(testfile, line))
    {
        wstring thisline;
        string tline = line;
        bool pan = StringToWString(line, thisline);
        if (pan)
        {
            for (int i = 0; i < thisline.length(); i++)
            {
                if ((thisline[i] >= 48 && thisline[i] <= 57) || (thisline[i] >= 65296 && thisline[i] <= 65305)||thisline[i]==L'○')
                {
                    switch (thisline[i])
                    {
                    case 48:
                    case 65296:
                    case L'○':
                        thisline[i] = L'零';
                        break;
                    case 49:
                    case 65297:
                        thisline[i] = L'一';
                        break;
                    case 50:
                    case 65298:
                        thisline[i] = L'二';
                        break;
                    case 51:
                    case 65299:
                        thisline[i] = L'三';
                        break;
                    case 52:
                    case 65300:
                        thisline[i] = L'四';
                        break;
                    case 53:
                    case 65301:
                        thisline[i] = L'五';
                        break;
                    case 54:
                    case 65302:
                        thisline[i] = L'六';
                        break;
                    case 55:
                    case 65303:
                        thisline[i] = L'七';
                        break;
                    case 56:
                    case 65304:
                        thisline[i] = L'八';
                        break;
                    case 57:
                    case 65305:
                        thisline[i] = L'九';
                        break;
                    default:
                        break;
                    }

                }
                if (thisline[i] == L'、' || thisline[i] == L'“' || thisline[i] == L'”'||thisline[i]==L'《'||thisline[i]==L'》')
                {
                    thisline[i] = ' ';
                }
                if (thisline[i] >= 0x4e00 && thisline[i] <= 0x9fbb || thisline[i] == ' ')
                {
                    if (thisline[i] != ' ')
                        temp += thisline[i];
                }
                else
                {

                    if (temp.length())
                    {
                       testoutput<< getanswer(temp)<<endl;
                    }
                    temp = L"";
                }
            }

            if (temp.length())
            {
                testoutput<<getanswer(temp)<<endl;
            }
            temp = L"";
        }
    }

    QMessageBox::information(NULL, u8"提示", u8"分词完成！", QMessageBox::Yes , QMessageBox::Yes);

}
