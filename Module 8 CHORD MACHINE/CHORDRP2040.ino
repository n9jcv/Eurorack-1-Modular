#include <hardware/pwm.h>
#include "pio_encoder.h"

//SSD1306 DISPLAY SETTINGS
#include<Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//ROTARY encoder setting
PioEncoder encoder(1); // P1 AND P2 ENCONDER - SPECIFY 1ST PIN
                       //2ND PIN IS ASSUMED SEQUENTIAL  CAN NOT CHANGE
//VARIOUS ENCODERS ARE SLIGHTLY DIFFERENT  SOME REQUIRE THE ENCODER.FLIP() COMMAND TO GET THE CORRECT TURNING DIRECTION - COMMENT OUT IF 
//IS REVERSED OUT  NOT HERE BUT BELOW IN CODE
//SOME ENCODERS ARE 4 COUNT AND SOME ARE 2 PER CLICK  CHANGE THE FOLLOWING LINE BELOW IN THE CODE FOR YOUR SPECIFIC ENCODER
//   NOT HERE BUT IN CODE BELOW          newencval = (encoder.getCount()/4);  CHANGE TO 2 OR 4 DEPENDING ON YOUR ENCODER

int menuitem = 0;
int tempmenuitem = 0;
int prevtempitem;
int prevmenuitem;
int newencval;  // THIS REPRESENTS THE ACTUAL COUNT FROM ENCODER 
                // THIS ENCODER REGISTERS 2 COUNTS FOR EACH CLICK
int oldencval;  // PREVIOUSLY READ VALUE FROM ENCODER
int encresult;  //result of read_enc() - generic so all functions can use the same encoder function
int prevencresult = 99; //previous final value from encoder
int topval;     //used to constrain the top range of the encoder

int newclock = 0;
int oldclock = 0;
int notecnt = 0;
int note[6];  //QUANTIZED VALUE OF NOTE TO BE PLAYED 0 TO 1023
char printnote[33] = "";  //USED TO FORMAT 2 DIGIT NUMBER WHEN PRINTING WITH SPRINTF

float osc_progress[6];//0~4=chord osc , 5=bass osc , 6=arpeggio osc
int osc_inverse[4];
float osc_scale_rate[5];//0~3=chord osc , 6=arpeggio osc
int slice_num = 0;
float osc_freq = 0;
int wavetable[256];//1024 resolution , 256 rate
int cvin;
int freq_pot=0; //NOT USING TUNE FUNCTION  THIS CAN ADJUST THE FREQUENCY UP 1 TO 127 TABLE ENTRIES
int source = 0;
int waveform = 0;
int prevwaveform;
int needoledupdate = 1;
int f0 = 35;//base osc frequency 30.6
int j=0;
int chord_type=1;//TO TUNE CV INPUT POT SET THIS TO 2  1 IS FOR GOLD MODULE CHORD WITH BASS
                 //DEFAULT 1 CHORD WITH ROOT - 0=chord without root , 1=chord with root,2=arpeggio

int pushsw = 1;
int old_pushsw = 1;
int pushcnt = 0;
int qnt[32];
int thr=0;//threshold number
const static int majqnt[32]={// major scale quantize value
0,  34, 68, 85, 119,  153,  205,  239,  273,  290,  324,  358,  409,  443,  477,  494,  529,  563,  614,  648,  682,  699,  733,  767,  818,  853,  887,  904,  938,  972,  1023
};

const static int majthr[32]={// major scale quantize threshold
 0, 17, 51, 85, 102,  136,  171,  222,  256,  290,  307,  341,  375,  426,  460,  494,  512,  546,  580,  631,  665,  699,  716,  750,  784,  835,  870,  904,  921,  955,  989,1024
};

const static int minqnt[32]={// minor scale quantize value
0,  34, 51, 85, 119,  136,  205,  239,  256,  290,  324,  341,  409,  443,  460,  494,  529,  546,  614,  648,  665,  699,  733,  750,  818,  853,  870,  904,  938,  955,  1023
};

const static int minthr[32]={// minor scale quantize threshold
 17,  43, 68, 102,  128,  171,  222,  248,  273,  307,  333,  375,  426,  452,  477,  512,  538,  580,  631,  657,  682,  716,  742,  784,  836,  862,  887,  921,  947,  989,  1024
};

//------------chord------------
float freq_rate[61]={
1,  1.059,  1.122,  1.189,  1.26, 1.335,  1.414,  1.498,  1.587,  1.682,  1.782,  1.888,  2,  2.059,  2.122,  2.189,  2.26, 2.335,  2.414,  2.498,  2.587,  2.682,  2.782,  2.888,  3,  3.059,  3.122,  3.189,  3.26, 3.335,  3.414,  3.498,  3.587,  3.682,  3.782,  3.888,  4,  4.059,  4.122,  4.189,  4.26, 4.335,  4.414,  4.498,  4.587,  4.682,  4.782,  4.888,  5,  5.059,  5.122,  5.189,  5.26, 5.335,  5.414,  5.498,  5.587,  5.682,  5.782,  5.888,  6
};

int chord_family=0;//0=maj 3chord,1=min 3chord,2=tension maj 4chord,3=maj 4chord,4=min 4chord,5=tension min 4chord
int prev_chord_family;

int chord_3[3][3]={
{0,4,7},//maj
{0,3,7},//min
{0,3,6}//dim
};

int chord_4[6][4]={
  {0,4,7,11},//maj+M7
  {0,2,4,7},//maj+add9
  {0,4,7,10},//maj+7
  {0,3,7,10},//min+7
  {0,4,7,9},//maj+6
  {0,3,6,10}//dim+7
};

int select_chord3[3][6]={//6 is mode kinds
  {0,1,1,0,0,0},//maj -> I,IIm,IIIm,IV,V,VIm
  {0,1,1,0,0,1},//maj -> I,IIm,IIIm,IV,V,VI
  {1,2,0,1,1,0}//min -> Im,IIdim,III,IVm,Vm,VI
};

int select_chord4[3][6]={//6 is mode kinds
  {1,3,3,0,4,1},//maj -> Iadd9,IIm7,IIIm7,IVM7,V6,VIadd9
  {0,3,3,0,2,3},//maj -> IM7,IIm7,IIIm7,IVM7,V7,VIm7
  {3,5,0,3,3,0}//min -> Im7,IIdim7,IIIM7,IVm7,Vm7,VIM7
};

int select_table=0;

//-----------V/oct-----------
const static float voctpow[1230] = {//Covers 6(=1230) octaves. If it is 1230 or more, the operation becomes unstable.
  0,  0.004882, 0.009765, 0.014648, 0.019531, 0.024414, 0.029296, 0.034179, 0.039062, 0.043945, 0.048828, 0.05371,  0.058593, 0.063476, 0.068359, 0.073242, 0.078125, 0.083007, 0.08789,  0.092773, 0.097656, 0.102539, 0.107421, 0.112304, 0.117187, 0.12207,  0.126953, 0.131835, 0.136718, 0.141601, 0.146484, 0.151367, 0.15625,  0.161132, 0.166015, 0.170898, 0.175781, 0.180664, 0.185546, 0.190429, 0.195312, 0.200195, 0.205078, 0.20996,  0.214843, 0.219726, 0.224609, 0.229492, 0.234375, 0.239257, 0.24414,  0.249023, 0.253906, 0.258789, 0.263671, 0.268554, 0.273437, 0.27832,  0.283203, 0.288085, 0.292968, 0.297851, 0.302734, 0.307617, 0.3125, 0.317382, 0.322265, 0.327148, 0.332031, 0.336914, 0.341796, 0.346679, 0.351562, 0.356445, 0.361328, 0.36621,  0.371093, 0.375976, 0.380859, 0.385742, 0.390625, 0.395507, 0.40039,  0.405273, 0.410156, 0.415039, 0.419921, 0.424804, 0.429687, 0.43457,  0.439453, 0.444335, 0.449218, 0.454101, 0.458984, 0.463867, 0.46875,  0.473632, 0.478515, 0.483398, 0.488281, 0.493164, 0.498046, 0.502929, 0.507812, 0.512695, 0.517578, 0.52246,  0.527343, 0.532226, 0.537109, 0.541992, 0.546875, 0.551757, 0.55664,  0.561523, 0.566406, 0.571289, 0.576171, 0.581054, 0.585937, 0.59082,  0.595703, 0.600585, 0.605468, 0.610351, 0.615234, 0.620117, 0.625,  0.629882, 0.634765, 0.639648, 0.644531, 0.649414, 0.654296, 0.659179, 0.664062, 0.668945, 0.673828, 0.67871,  0.683593, 0.688476, 0.693359, 0.698242, 0.703125, 0.708007, 0.71289,  0.717773, 0.722656, 0.727539, 0.732421, 0.737304, 0.742187, 0.74707,  0.751953, 0.756835, 0.761718, 0.766601, 0.771484, 0.776367, 0.78125,  0.786132, 0.791015, 0.795898, 0.800781, 0.805664, 0.810546, 0.815429, 0.820312, 0.825195, 0.830078, 0.83496,  0.839843, 0.844726, 0.849609, 0.854492, 0.859375, 0.864257, 0.86914,  0.874023, 0.878906, 0.883789, 0.888671, 0.893554, 0.898437, 0.90332,  0.908203, 0.913085, 0.917968, 0.922851, 0.927734, 0.932617, 0.9375, 0.942382, 0.947265, 0.952148, 0.957031, 0.961914, 0.966796, 0.971679, 0.976562, 0.981445, 0.986328, 0.99121,  0.996093, 1.000976, 1.005859, 1.010742, 1.015625, 1.020507, 1.02539,  1.030273, 1.035156, 1.040039, 1.044921, 1.049804, 1.054687, 1.05957,  1.064453, 1.069335, 1.074218, 1.079101, 1.083984, 1.088867, 1.09375,  1.098632, 1.103515, 1.108398, 1.113281, 1.118164, 1.123046, 1.127929, 1.132812, 1.137695, 1.142578, 1.14746,  1.152343, 1.157226, 1.162109, 1.166992, 1.171875, 1.176757, 1.18164,  1.186523, 1.191406, 1.196289, 1.201171, 1.206054, 1.210937, 1.21582,  1.220703, 1.225585, 1.230468, 1.235351, 1.240234, 1.245117, 1.25, 1.254882, 1.259765, 1.264648, 1.269531, 1.274414, 1.279296, 1.284179, 1.289062, 1.293945, 1.298828, 1.30371,  1.308593, 1.313476, 1.318359, 1.323242, 1.328125, 1.333007, 1.33789,  1.342773, 1.347656, 1.352539, 1.357421, 1.362304, 1.367187, 1.37207,  1.376953, 1.381835, 1.386718, 1.391601, 1.396484, 1.401367, 1.40625,  1.411132, 1.416015, 1.420898, 1.425781, 1.430664, 1.435546, 1.440429, 1.445312, 1.450195, 1.455078, 1.45996,  1.464843, 1.469726, 1.474609, 1.479492, 1.484375, 1.489257, 1.49414,  1.499023, 1.503906, 1.508789, 1.513671, 1.518554, 1.523437, 1.52832,  1.533203, 1.538085, 1.542968, 1.547851, 1.552734, 1.557617, 1.5625, 1.567382, 1.572265, 1.577148, 1.582031, 1.586914, 1.591796, 1.596679, 1.601562, 1.606445, 1.611328, 1.61621,  1.621093, 1.625976, 1.630859, 1.635742, 1.640625, 1.645507, 1.65039,  1.655273, 1.660156, 1.665039, 1.669921, 1.674804, 1.679687, 1.68457,  1.689453, 1.694335, 1.699218, 1.704101, 1.708984, 1.713867, 1.71875,  1.723632, 1.728515, 1.733398, 1.738281, 1.743164, 1.748046, 1.752929, 1.757812, 1.762695, 1.767578, 1.77246,  1.777343, 1.782226, 1.787109, 1.791992, 1.796875, 1.801757, 1.80664,  1.811523, 1.816406, 1.821289, 1.826171, 1.831054, 1.835937, 1.84082,  1.845703, 1.850585, 1.855468, 1.860351, 1.865234, 1.870117, 1.875,  1.879882, 1.884765, 1.889648, 1.894531, 1.899414, 1.904296, 1.909179, 1.914062, 1.918945, 1.923828, 1.92871,  1.933593, 1.938476, 1.943359, 1.948242, 1.953125, 1.958007, 1.96289,  1.967773, 1.972656, 1.977539, 1.982421, 1.987304, 1.992187, 1.99707,  2.001953, 2.006835, 2.011718, 2.016601, 2.021484, 2.026367, 2.03125,  2.036132, 2.041015, 2.045898, 2.050781, 2.055664, 2.060546, 2.065429, 2.070312, 2.075195, 2.080078, 2.08496,  2.089843, 2.094726, 2.099609, 2.104492, 2.109375, 2.114257, 2.11914,  2.124023, 2.128906, 2.133789, 2.138671, 2.143554, 2.148437, 2.15332,  2.158203, 2.163085, 2.167968, 2.172851, 2.177734, 2.182617, 2.1875, 2.192382, 2.197265, 2.202148, 2.207031, 2.211914, 2.216796, 2.221679, 2.226562, 2.231445, 2.236328, 2.24121,  2.246093, 2.250976, 2.255859, 2.260742, 2.265625, 2.270507, 2.27539,  2.280273, 2.285156, 2.290039, 2.294921, 2.299804, 2.304687, 2.30957,  2.314453, 2.319335, 2.324218, 2.329101, 2.333984, 2.338867, 2.34375,  2.348632, 2.353515, 2.358398, 2.363281, 2.368164, 2.373046, 2.377929, 2.382812, 2.387695, 2.392578, 2.39746,  2.402343, 2.407226, 2.412109, 2.416992, 2.421875, 2.426757, 2.43164,  2.436523, 2.441406, 2.446289, 2.451171, 2.456054, 2.460937, 2.46582,  2.470703, 2.475585, 2.480468, 2.485351, 2.490234, 2.495117, 2.5,  2.504882, 2.509765, 2.514648, 2.519531, 2.524414, 2.529296, 2.534179, 2.539062, 2.543945, 2.548828, 2.55371,  2.558593, 2.563476, 2.568359, 2.573242, 2.578125, 2.583007, 2.58789,  2.592773, 2.597656, 2.602539, 2.607421, 2.612304, 2.617187, 2.62207,  2.626953, 2.631835, 2.636718, 2.641601, 2.646484, 2.651367, 2.65625,  2.661132, 2.666015, 2.670898, 2.675781, 2.680664, 2.685546, 2.690429, 2.695312, 2.700195, 2.705078, 2.70996,  2.714843, 2.719726, 2.724609, 2.729492, 2.734375, 2.739257, 2.74414,  2.749023, 2.753906, 2.758789, 2.763671, 2.768554, 2.773437, 2.77832,  2.783203, 2.788085, 2.792968, 2.797851, 2.802734, 2.807617, 2.8125, 2.817382, 2.822265, 2.827148, 2.832031, 2.836914, 2.841796, 2.846679, 2.851562, 2.856445, 2.861328, 2.86621,  2.871093, 2.875976, 2.880859, 2.885742, 2.890625, 2.895507, 2.90039,  2.905273, 2.910156, 2.915039, 2.919921, 2.924804, 2.929687, 2.93457,  2.939453, 2.944335, 2.949218, 2.954101, 2.958984, 2.963867, 2.96875,  2.973632, 2.978515, 2.983398, 2.988281, 2.993164, 2.998046, 3.002929, 3.007812, 3.012695, 3.017578, 3.02246,  3.027343, 3.032226, 3.037109, 3.041992, 3.046875, 3.051757, 3.05664,  3.061523, 3.066406, 3.071289, 3.076171, 3.081054, 3.085937, 3.09082,  3.095703, 3.100585, 3.105468, 3.110351, 3.115234, 3.120117, 3.125,  3.129882, 3.134765, 3.139648, 3.144531, 3.149414, 3.154296, 3.159179, 3.164062, 3.168945, 3.173828, 3.17871,  3.183593, 3.188476, 3.193359, 3.198242, 3.203125, 3.208007, 3.21289,  3.217773, 3.222656, 3.227539, 3.232421, 3.237304, 3.242187, 3.24707,  3.251953, 3.256835, 3.261718, 3.266601, 3.271484, 3.276367, 3.28125,  3.286132, 3.291015, 3.295898, 3.300781, 3.305664, 3.310546, 3.315429, 3.320312, 3.325195, 3.330078, 3.33496,  3.339843, 3.344726, 3.349609, 3.354492, 3.359375, 3.364257, 3.36914,  3.374023, 3.378906, 3.383789, 3.388671, 3.393554, 3.398437, 3.40332,  3.408203, 3.413085, 3.417968, 3.422851, 3.427734, 3.432617, 3.4375, 3.442382, 3.447265, 3.452148, 3.457031, 3.461914, 3.466796, 3.471679, 3.476562, 3.481445, 3.486328, 3.49121,  3.496093, 3.500976, 3.505859, 3.510742, 3.515625, 3.520507, 3.52539,  3.530273, 3.535156, 3.540039, 3.544921, 3.549804, 3.554687, 3.55957,  3.564453, 3.569335, 3.574218, 3.579101, 3.583984, 3.588867, 3.59375,  3.598632, 3.603515, 3.608398, 3.613281, 3.618164, 3.623046, 3.627929, 3.632812, 3.637695, 3.642578, 3.64746,  3.652343, 3.657226, 3.662109, 3.666992, 3.671875, 3.676757, 3.68164,  3.686523, 3.691406, 3.696289, 3.701171, 3.706054, 3.710937, 3.71582,  3.720703, 3.725585, 3.730468, 3.735351, 3.740234, 3.745117, 3.75, 3.754882, 3.759765, 3.764648, 3.769531, 3.774414, 3.779296, 3.784179, 3.789062, 3.793945, 3.798828, 3.80371,  3.808593, 3.813476, 3.818359, 3.823242, 3.828125, 3.833007, 3.83789,  3.842773, 3.847656, 3.852539, 3.857421, 3.862304, 3.867187, 3.87207,  3.876953, 3.881835, 3.886718, 3.891601, 3.896484, 3.901367, 3.90625,  3.911132, 3.916015, 3.920898, 3.925781, 3.930664, 3.935546, 3.940429, 3.945312, 3.950195, 3.955078, 3.95996,  3.964843, 3.969726, 3.974609, 3.979492, 3.984375, 3.989257, 3.99414,  3.999023, 4.003906, 4.008789, 4.013671, 4.018554, 4.023437, 4.02832,  4.033203, 4.038085, 4.042968, 4.047851, 4.052734, 4.057617, 4.0625, 4.067382, 4.072265, 4.077148, 4.082031, 4.086914, 4.091796, 4.096679, 4.101562, 4.106445, 4.111328, 4.11621,  4.121093, 4.125976, 4.130859, 4.135742, 4.140625, 4.145507, 4.15039,  4.155273, 4.160156, 4.165039, 4.169921, 4.174804, 4.179687, 4.18457,  4.189453, 4.194335, 4.199218, 4.204101, 4.208984, 4.213867, 4.21875,  4.223632, 4.228515, 4.233398, 4.238281, 4.243164, 4.248046, 4.252929, 4.257812, 4.262695, 4.267578, 4.27246,  4.277343, 4.282226, 4.287109, 4.291992, 4.296875, 4.301757, 4.30664,  4.311523, 4.316406, 4.321289, 4.326171, 4.331054, 4.335937, 4.34082,  4.345703, 4.350585, 4.355468, 4.360351, 4.365234, 4.370117, 4.375,  4.379882, 4.384765, 4.389648, 4.394531, 4.399414, 4.404296, 4.409179, 4.414062, 4.418945, 4.423828, 4.42871,  4.433593, 4.438476, 4.443359, 4.448242, 4.453125, 4.458007, 4.46289,  4.467773, 4.472656, 4.477539, 4.482421, 4.487304, 4.492187, 4.49707,  4.501953, 4.506835, 4.511718, 4.516601, 4.521484, 4.526367, 4.53125,  4.536132, 4.541015, 4.545898, 4.550781, 4.555664, 4.560546, 4.565429, 4.570312, 4.575195, 4.580078, 4.58496,  4.589843, 4.594726, 4.599609, 4.604492, 4.609375, 4.614257, 4.61914,  4.624023, 4.628906, 4.633789, 4.638671, 4.643554, 4.648437, 4.65332,  4.658203, 4.663085, 4.667968, 4.672851, 4.677734, 4.682617, 4.6875, 4.692382, 4.697265, 4.702148, 4.707031, 4.711914, 4.716796, 4.721679, 4.726562, 4.731445, 4.736328, 4.74121,  4.746093, 4.750976, 4.755859, 4.760742, 4.765625, 4.770507, 4.77539,  4.780273, 4.785156, 4.790039, 4.794921, 4.799804, 4.804687, 4.80957,  4.814453, 4.819335, 4.824218, 4.829101, 4.833984, 4.838867, 4.84375,  4.848632, 4.853515, 4.858398, 4.863281, 4.868164, 4.873046, 4.877929, 4.882812, 4.887695, 4.892578, 4.89746,  4.902343, 4.907226, 4.912109, 4.916992, 4.921875, 4.926757, 4.93164,  4.936523, 4.941406, 4.946289, 4.951171, 4.956054, 4.960937, 4.96582,  4.970703, 4.975585, 4.980468, 4.985351, 4.990234, 4.995117, 5,  5.004882, 5.009765, 5.014648, 5.019531, 5.024414, 5.029296, 5.034179, 5.039062, 5.043945, 5.048828, 5.05371,  5.058593, 5.063476, 5.068359, 5.073242, 5.078125, 5.083007, 5.08789,  5.092773, 5.097656, 5.102539, 5.107421, 5.112304, 5.117187, 5.12207,  5.126953, 5.131835, 5.136718, 5.141601, 5.146484, 5.151367, 5.15625,  5.161132, 5.166015, 5.170898, 5.175781, 5.180664, 5.185546, 5.190429, 5.195312, 5.200195, 5.205078, 5.20996,  5.214843, 5.219726, 5.224609, 5.229492, 5.234375, 5.239257, 5.24414,  5.249023, 5.253906, 5.258789, 5.263671, 5.268554, 5.273437, 5.27832,  5.283203, 5.288085, 5.292968, 5.297851, 5.302734, 5.307617, 5.3125, 5.317382, 5.322265, 5.327148, 5.332031, 5.336914, 5.341796, 5.346679, 5.351562, 5.356445, 5.361328, 5.36621,  5.371093, 5.375976, 5.380859, 5.385742, 5.390625, 5.395507, 5.40039,  5.405273, 5.410156, 5.415039, 5.419921, 5.424804, 5.429687, 5.43457,  5.439453, 5.444335, 5.449218, 5.454101, 5.458984, 5.463867, 5.46875,  5.473632, 5.478515, 5.483398, 5.488281, 5.493164, 5.498046, 5.502929, 5.507812, 5.512695, 5.517578, 5.52246,  5.527343, 5.532226, 5.537109, 5.541992, 5.546875, 5.551757, 5.55664,  5.561523, 5.566406, 5.571289, 5.576171, 5.581054, 5.585937, 5.59082,  5.595703, 5.600585, 5.605468, 5.610351, 5.615234, 5.620117, 5.625,  5.629882, 5.634765, 5.639648, 5.644531, 5.649414, 5.654296, 5.659179, 5.664062, 5.668945, 5.673828, 5.67871,  5.683593, 5.688476, 5.693359, 5.698242, 5.703125, 5.708007, 5.71289,  5.717773, 5.722656, 5.727539, 5.732421, 5.737304, 5.742187, 5.74707,  5.751953, 5.756835, 5.761718, 5.766601, 5.771484, 5.776367, 5.78125,  5.786132, 5.791015, 5.795898, 5.800781, 5.805664, 5.810546, 5.815429, 5.820312, 5.825195, 5.830078, 5.83496,  5.839843, 5.844726, 5.849609, 5.854492, 5.859375, 5.864257, 5.86914,  5.874023, 5.878906, 5.883789, 5.888671, 5.893554, 5.898437, 5.90332,  5.908203, 5.913085, 5.917968, 5.922851, 5.927734, 5.932617, 5.9375, 5.942382, 5.947265, 5.952148, 5.957031, 5.961914, 5.966796, 5.971679, 5.976562, 5.981445, 5.986328, 5.99121,  5.996093, 6.000976,
};
float freq_table[2048];

void setup1() {
}

void setup()
{  
  //Serial.println("SETUP");
  old_pushsw = 1;

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("    CHORD MACHINE");
  display.setCursor(0, 18);
  display.println("  00 00 00 00 00 00");  
  display.display();
  source = 0;
  menuitem = 7;
  refresh_oled();
  waveform = 0;
  menuitem = 8;
  refresh_oled();
  menuitem = 9;
  chord_family =0;
  refresh_oled();
  menuitem = 0;
  
  //ENCODER SETUP
  encoder.begin();
  //USE AS NEEDED   encoder.flip();

  pinMode(0, INPUT);//CLOCK IN
  pinMode(3, INPUT_PULLUP);//ENCODER SWITCH

  table_set();//set wavetable
  qnt_set();
  //-------------------octave select-------------------------------
  for (int i = 0; i < 1230; i++) {//Covers 6(=1230) octaves. If it is 1230 or more, the operation becomes unstable.
    freq_table[i] = f0 * pow(2, (voctpow[i]));
  }
  for (int i = 0; i < 2048 - 1230; i++) {
    freq_table[i + 1230] = 6;
  }

  //-------------------PWM setting-------------------------------
  gpio_set_function(4, GPIO_FUNC_PWM);// ON SEEED XIAO RP2040 - AUDIO OUT ON PIN D9/P4 - function PWM GP4
  slice_num = pwm_gpio_to_slice_num(4);// GP4 PWM slice

  pwm_clear_irq(slice_num);
  pwm_set_irq_enabled(slice_num, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
  irq_set_enabled(PWM_IRQ_WRAP, true);

  //set PWM frequency
  pwm_set_clkdiv(slice_num, 4);//=sysclock/((resolution+1)*frequency)
  pwm_set_wrap(slice_num, 1023);//resolutio
  pwm_set_enabled(slice_num, true);//PWM output enable

  osc_inverse[0]=1;
  osc_inverse[1]=1;
  osc_inverse[2]=1;
  osc_scale_rate[4]=osc_scale_rate[0];
}

void  on_pwm_wrap() {
  
  pwm_clear_irq(slice_num);
  //APPREGIO LINES BELOW USED ONLY FOR TESTING TO TUNE CV INPUT ON MODULE AND ENSURE IN TUNE
  if(chord_family==2 ||chord_family==3 ||chord_family==4){
    osc_progress[0]=osc_progress[0]+  osc_freq*osc_scale_rate[0];
    osc_progress[1]=osc_progress[1]+  osc_freq*osc_scale_rate[1];
    osc_progress[2]=osc_progress[2]+  osc_freq*osc_scale_rate[2];
    osc_progress[4]=osc_progress[4]+  osc_freq*osc_scale_rate[0]/2;//bass
    osc_progress[5]=osc_progress[5]+  osc_freq*osc_scale_rate[0]/2;//arpeggio WAS 4 SET TO ZERO TO GET LOW NOTE FOR TESTING
    }
  else if(chord_family==0 ||chord_family==1||chord_family==5){
    osc_progress[0]=osc_progress[0]+  osc_freq*osc_scale_rate[0];
    osc_progress[1]=osc_progress[1]+  osc_freq*osc_scale_rate[1];
    osc_progress[2]=osc_progress[2]+  osc_freq*osc_scale_rate[2];
    osc_progress[3]=osc_progress[3]+  osc_freq*osc_scale_rate[3];
    osc_progress[4]=osc_progress[4]+  osc_freq*osc_scale_rate[0]/2;//bass
    osc_progress[5]=osc_progress[5]+  osc_freq*osc_scale_rate[0]/2;//arpeggio was 4
  }
  for(byte i = 0; i < 6; i++){
    if (osc_progress[i] > 255) {
    osc_progress[i] = 0;
    }
  }

  //  MODIFIED ONLY SUPPORTS CHORD TYPE 1
  if((chord_family==2 ||chord_family==3 ||chord_family==4)&& chord_type==1){
    pwm_set_chan_level(slice_num, PWM_CHAN_A, wavetable[(int)osc_progress[0]]/4+wavetable[(int)osc_progress[1]]/4+wavetable[(int)osc_progress[2]]/4+wavetable[(int)osc_progress[4]]/4+511);
  }
  else if((chord_family==0 ||chord_family==1||chord_family==5) && chord_type==1){
    pwm_set_chan_level(slice_num, PWM_CHAN_A, wavetable[(int)osc_progress[0]]/5+wavetable[(int)osc_progress[1]]/5+wavetable[(int)osc_progress[2]]/5+wavetable[(int)osc_progress[3]]/5+wavetable[(int)osc_progress[4]]/5+511);
  }
  // CHORD TYPE 2 IS APPREGIO SINGLE NOTE CAN BE USED TO TUNE THE CV FOR THE MODULE WITH HARDWARE POT FOR CV
  else if(chord_type==2){
    pwm_set_chan_level(slice_num, PWM_CHAN_A, wavetable[(int)osc_progress[5]]/2+511);
  }

}
  
void loop()
{
  //Serial.begin(115200);//for development
  //MENU ITEMS
  // 0 - MAIN MENU NAVIGATE
  // 1 - NOTE 1  // 2 - NOTE 2  // 3 - NOTE 3  // 4 - NOTE 4  // 5 - NOTE 5
  // 6 - NOTE 6  // 7 - SOURCE  // 8 - WAVE  // 9 - CHORD
  //CHECK PUSH SWITCH AND ENCODER
  old_pushsw = pushsw;
  pushsw = digitalRead(3);
  //IF BEGIN OF A NEW PUSH SET COUNT = 0
  if ((pushsw == 0) and (old_pushsw == 1)){
    pushcnt =0;
  }
  if (pushsw == 0){
    pushcnt ++;
  }

  read_enc();
  
  //PROCESS SWITCH PUSHES
  if ((pushcnt < 20000) and (old_pushsw == 0) and (pushsw == 1) and (menuitem == 0)){
      //Serial.println("chg menu item");
      menuitem = tempmenuitem;
      //needoledupdate = 1;
      encoder.reset();
      tempmenuitem = 0;
      encresult = 0;
      prevencresult = 0;
      refresh_oled();
  }
  else if ((pushcnt > 20000) and (old_pushsw == 0) and (pushsw == 1) and (menuitem != 0)){
      //Serial.println("set men zero");
      menuitem = 0;
      encoder.reset();
      tempmenuitem = 0;
      encresult = 0;
      prevencresult = 0;
      refresh_oled();
      //Serial.println(encoder.getCount()/2);      
  }
             
  //SET CONSTRAINTS AND VALUES FOR EACH MENU ITEM 
  //ONLY DO WHEN ENCODER RESULT CHANGES
  if (prevencresult != encresult){
    prevencresult = encresult;
    switch(menuitem){
        case 0:
        topval = 9;
        tempmenuitem = encresult;
        break;
        case 1:
        topval = 31;
        break;
        case 2:
        topval = 31;
        break;    
        case 3:
        topval = 31;
        break;
        case 4:
        topval = 31;
        break;
        case 5:
        topval = 31;
        break;
        case 6:
        topval = 31;
        break;
        case 7:
        topval = 1;
        source = encresult;
        break;
        case 8:
        topval = 7;            
        waveform = encresult;
        break;
        case 9:
        topval = 5;
        chord_family = encresult;
        break;            
    }
  } 

  //  -------------------chord family-------------------------------
  //NOT USING INVERSIONS CANT DELETE USE ZERO INVERSION AS DEFAULT
  //---------SELECT CHORD TABLE SET TABLE/OSC CHANGES----------------
  if (chord_family != prev_chord_family) {
      osc_inverse[0]=1;
      osc_inverse[1]=1;
      osc_inverse[2]=1;
      osc_scale_rate[4]=osc_scale_rate[0];
      prev_chord_family = chord_family;      
      switch(chord_family){
         case 0:         
         osc_inverse[3]=1;
         select_table=0;//4 chord
         break;
         case 1:
         osc_inverse[3]=1;
         select_table=1;//4 chord
         break;
         case 2:
         select_table=0;//3 chord
         break;
         case 3:
         select_table=1;//3 chord
         break;
         case 4:         
         select_table=2;//3 chord
         break;
         case 5:         
         osc_inverse[3]=1;
         select_table=2;//4 chord
         break;
    }
  }

  //  -------------------frequeny calculation-------------------------------
  //  IF SOURCE INTERNAL DO NOT READ CVIN   CVIN WILL BE SET FROM NOTES SELECTED //
  //READ AND PROCESS CLOCK IN - ONLY QUANTIZE NOTE ON NEW RISING CLOCK AND NOT AGAIN UNTIL A NEW RISING CLOCK  
  newclock = digitalRead(0);  
  if ((newclock == 1) and (oldclock == 0)){                  
      //oldclock = newclock;
      if (source == 0){//IF SOURCE IS CV READ CV IN VALUE P26
        cvin = analogRead(26); 
        qnt_set();              
      }
      else if (source == 1){//IF SOURCE IS INTERNAL THEN SELECT NOTE 1 TO 6 AND CYLCLE THRU WITH CLOCK
              getnote();
              qnt_set();
              //COMMENTED OUT   THIS DRAWLINE/SCREEN REFRESH TAKES 33ms AND THIS CAUSES CHIRP WITH INTERNAL NOTE 
              //DUE TO DELAY CAUSE BY MEMORY DUMP TO SCREEN   MOVED FURTHER BELOW TO AVOID
              //  drawline();             
      }      
  }
  
  if ((newclock == 0) and (oldclock == 1)){
      oldclock = newclock;
  }


  //SOUND PROCESSING
  if (prevwaveform != waveform){
     prevwaveform = waveform;
     table_set();//set wavetable
  }

  //freq_pot = map(analogRead(27), 0, 1023, 0, 127); 
  osc_freq = freq_table[qnt[thr] + freq_pot]; // V/oct apply
  osc_freq = 256 * osc_freq / 122070 * 8;//7 is base octave

  //select oscillator frequencies based on chord family
  if(chord_family==2 ||chord_family==3 ||chord_family==4){//set each oscillator frequency
    osc_scale_rate[0]=freq_rate[chord_3[select_chord3[select_table][thr%6]][0]]*osc_inverse[0];
    osc_scale_rate[1]=freq_rate[chord_3[select_chord3[select_table][thr%6]][1]]*osc_inverse[1];
    osc_scale_rate[2]=freq_rate[chord_3[select_chord3[select_table][thr%6]][2]]*osc_inverse[2];
  };
  if(chord_family==0 ||chord_family==1||chord_family==5){
    osc_scale_rate[0]=freq_rate[chord_4[select_chord4[select_table][thr%6]][0]]*osc_inverse[0];
    osc_scale_rate[1]=freq_rate[chord_4[select_chord4[select_table][thr%6]][1]]*osc_inverse[1];
    osc_scale_rate[2]=freq_rate[chord_4[select_chord4[select_table][thr%6]][2]]*osc_inverse[2];
    osc_scale_rate[3]=freq_rate[chord_4[select_chord4[select_table][thr%6]][3]]*osc_inverse[3];
  };

//THIS WAS MOVED FROM ABOVE BECAUSE OF A RESOURCE BLOCKING  CAUSING CHIRPING WHEN INTERNAL AND CHANGING CHORD
  if ((newclock == 1) and (oldclock == 0)){                  
      oldclock = newclock;
      if (source == 1){//IF SOURCE IS CV READ CV IN VALUE P26
        drawline();             
      }
  }

 if (old_pushsw == 0 && pushsw == 1) {
   pushcnt = 0;
 }

 if (needoledupdate == 1){
    refresh_oled();
    needoledupdate =0;  
 }

}

void getnote(){
  if (notecnt < 5){
      notecnt++;
  }
  else if (notecnt == 5){
           notecnt = 0;
  }
  if (note[notecnt] != 0){//00 WILL NOT SOUND - USE TO HOLD A CHORD FROM PREVIOUS
      cvin = note[notecnt];
  }    
}

void drawline(){//DRAWS LINE UNDER NOTE PLAYING WHEN SOURCE IS INTERNAL
  //xst, yst, xen, yen
  display.drawLine(0, 27, 126, 27, BLACK);
  display.drawLine(11+(notecnt*18), 27, 22+(notecnt*18), 27, WHITE);
  display.display();
  
}

void qnt_set(){//quantize v/oct input
   // Serial.println("QNT");
  if(chord_family==0 ||chord_family==1 ||chord_family==2||chord_family==3){//major scale
    for(j=0;j<31;j++){
      if(cvin>=majthr[j] && cvin<majthr[j+1]){
        thr=j;
      }
    }
    for(j=0;j<31;j++){
      qnt[j]=majqnt[j];
    }
  }

 if(chord_family==4 ||chord_family==5){//minor scale
   for(j=0;j<31;j++){
      if(cvin>=minthr[j] && cvin<minthr[j+1]){
        thr=j;
      }
    }
    for(j=0;j<31;j++){
      qnt[j]=minqnt[j];
    }
  }
}

void read_enc(){
      //READ ENCODER
      newencval = (encoder.getCount()/4);
      if (newencval > oldencval) {
        encresult ++;
        needoledupdate = 1;
      }
      else if (newencval < oldencval) {
        encresult --;
        needoledupdate = 1;
      }
      if ((encresult < 0) or (encresult > topval)){//DONT UPDATE OLED IF TRYING TO TURN PAST UPPER OR LOWER LIMIT WASTE OF TIME
        needoledupdate = 0;
      }
      oldencval = newencval;
      encresult = constrain(encresult, 0, topval);
}

void refresh_oled(){
    //UPDATE OLED WITH CHANGES TO VALUES
    //PUT ALL LABELS HERE AND PRINT WHITE ON BLACK  
    //THEN IF A MENU ITEM IS SELECTED BELOW WILL HIGHLIGHT THE ITEM WHILE SELECTED
    //Serial.println("REFRESH OLED");
    display.setTextColor(WHITE, BLACK);
    display.setCursor(0, 9);
    display.println("  N1 N2 N3 N4 N5 N6");
    display.setCursor(0, 32);
    display.println("SOURCE");
    display.setCursor(0, 42);
    display.println("WAVE");
    display.setCursor(0, 52);   
    display.println("CHORD");
    //SET COLOR BACK TO NORMAL FOR FOLLOWING
    display.setTextColor(WHITE, BLACK);

    if (menuitem == 0){
        tempmenuitem = constrain(tempmenuitem, 1, 9);      
    }

    //N1
    if (menuitem == 0){
      display.setTextColor(BLACK, WHITE);
      switch (tempmenuitem){
      case 1:
      display.setCursor(11, 9);
      display.print("N1");
      break;
      case 2:
      display.setCursor(29, 9);
      display.print("N2");
      break;
      case 3:
      display.setCursor(47, 9);
      display.print("N3");
      break;
      case 4:
      display.setCursor(65, 9);
      display.print("N4");
      break;
      case 5:
      display.setCursor(83, 9);
      display.print("N5");
      break;
      case 6:
      display.setCursor(101, 9);
      display.print("N6");
      break;
      case 7:
      display.setCursor(0, 32);
      display.print("SOURCE");
      break;
      case 8:
      display.setCursor(0, 42);
      display.println("WAVE");
      break;
      case 9:
      display.setCursor(0, 52);
      display.println("CHORD");
      break;
      }
    }       
    //N1
    else if (menuitem == 1){
      display.setCursor(11, 18);
      note[0] = qnt[encresult];
      sprintf(printnote, "%02d", encresult);
      display.print(printnote);
    }
    //N2
    else if (menuitem == 2){
      display.setCursor(29, 18);
      note[1] = qnt[encresult];
      sprintf(printnote, "%02d", encresult);
      display.print(printnote);
    }    
    //N3
    else if (menuitem == 3){
      display.setCursor(47, 18);
      note[2] = qnt[encresult];
      sprintf(printnote, "%02d", encresult);
      display.print(printnote);
    }    
    //N4
    else if (menuitem == 4){
      display.setCursor(65, 18);
      note[3] = qnt[encresult];
      sprintf(printnote, "%02d", encresult);
      display.print(printnote);
    }    
    //N5
    else if (menuitem == 5){
      display.setCursor(83, 18);
      note[4] = qnt[encresult];
      sprintf(printnote, "%02d", encresult);
      display.print(printnote);
    }
    //N6
    else if (menuitem == 6){
      display.setCursor(101, 18);
      note[5] = qnt[encresult];
      sprintf(printnote, "%02d", encresult);
      display.print(printnote);
    }    
    //SOURCE CV OR INTERNAL
    else if (menuitem == 7){
      //Serial.println("SOURCE");
      display.setCursor(48, 32);
      switch (source){
        case 0:
        display.print("CV      ");
        //Serial.println("CV");
        break;
        case 1:
        display.print("INTERNAL");
        notecnt = 0;
        break;        
      }
    }
    //waveform stuff
    //PROCESS WAVEFORM CHANGE 
    else if (menuitem == 8){
      display.setCursor(48, 42);
      switch(waveform){
        case 0:
        display.print("SAW       ");
        break;
        case 1:
        display.print("SINE      ");
        break;
        case 2:
        display.print("SQUARE    ");
        break;    
        case 3:
        display.print("TRIANGLE  ");
        break;
        case 4:
        display.print("OCTAVE SAW");
        break;
        case 5:
        display.print("FM1       ");
        break;
        case 6:
        display.print("FM2       ");
        break;
        case 7:
        display.print("FM3       ");
        break;    
      } 
    }
    //CHORD FAMILY MENU STUFF  
    else if (menuitem == 9){
          display.setCursor(48, 52);
          switch(chord_family){
             case 0:
             display.print("MAJOR 1");      
             break;
             case 1:
             display.print("MAJOR 2");         
             break;
             case 2:
             display.print("MAJOR 3");        
             break;
             case 3:
             display.print("MAJOR 4");         
             break;
             case 4:
             display.print("MINOR 1");         
             break;
             case 5:
             display.print("MINOR 2");        
             break;
          }              
        }     
    display.display();
}

void table_set() {//make wavetable
   
  switch  (waveform) {
    case 0:
        for (int i = 0; i < 256; i++) {  //saw
          wavetable[i] = i * 4 - 512;
        }
        break;

    case 1:
        for (int i = 0; i < 256; i++) {  //sin
          wavetable[i] = (sin(2 * M_PI * i  / 256)) * 511;
        }
        break;

    case 2:
        for (int i = 0; i < 128; i++) {  //squ
          wavetable[i] = 511;
          wavetable[i + 128] = -511;
        }
        break;

    case 3:
        for (int i = 0; i < 128; i++) {  //tri
          wavetable[i] = i * 8 - 511;
          wavetable[i + 128] = 511 - i * 8;
        }
        break;

    case 4:
        for (int i = 0; i < 128; i++) {  //oct saw
          wavetable[i] = i * 4 - 512 + i * 2;
          wavetable[i + 128] = i * 2 - 256 + i * 4;
        }
        break;

    case 5:
        for (int i = 0; i < 256; i++) {  //FM1
          wavetable[i] = (sin(2 * M_PI * i  / 256 + sin(2 * M_PI * 3 * i  / 256)) ) * 511;
        }
        break;

    case 6:
        for (int i = 0; i < 256; i++) {  //FM2
          wavetable[i] = (sin(2 * M_PI * i  / 256 + sin(2 * M_PI * 7 * i  / 256))) * 511;
        }
        break;

    case 7:
        for (int i = 0; i < 256; i++) {  //FM3
          wavetable[i] = (sin(2 * M_PI * i  / 256 + sin(2 * M_PI * 4 * i  / 256 + sin(2 * M_PI * 11 * i  / 256)))) * 511;
        }
        break;
  }
}
