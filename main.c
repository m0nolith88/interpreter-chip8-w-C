#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define H 64
#define W 32
#define MAGNIFICATION 8

unsigned char memory[4096];
char V[16]={0};
short I=0;
short PC=0x200;
char SP=0;
short stack[16]={0};

char font[80]={
    0xF0,0x90,0x90,0x90,0xF0,
    0x20,0x60,0x20,0x20,0x70,
    0xF0,0x10,0xF0,0x80,0xF0,
    0xF0,0x10,0xF0,0x10,0xF0,
    0x90,0x90,0xF0,0x10,0x10,
    0xF0,0x80,0xF0,0x10,0xF0,
    0xF0,0x80,0xF0,0x90,0xF0,
    0xF0,0x10,0x20,0x40,0x40,
    0xF0,0x90,0xF0,0x90,0xF0,
    0xF0,0x90,0xF0,0x10,0xF0,
    0xF0,0x90,0xF0,0x90,0x90,
    0xE0,0x90,0xE0,0x90,0xE0,
    0xF0,0x80,0x80,0x80,0xF0,
    0xE0,0x90,0x90,0x90,0xE0,
    0xF0,0x80,0xF0,0x80,0xF0,
    0xF0,0x80,0xF0,0x80,0x80
};

char gfx[64][32]={0};

char keyboardCh[16]={
    '1','2','3',
    '4','5','6',
    '7','8','0',
    'a','b','c',
    'd','e','f'
};

long int fsize=0;

bool jumpPerformed=false;

void load(char *filename);
void loadFont();
void emulate();

//Ogl section

#define checkImageWidth 64
#define checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];

static GLuint texName;

void keyboard(unsigned char key,int x,int y);
void reshape(int w,int h);
void display(void);
void init(void);
void makeCheckImage(void);

void makeCheckImage(void){
    int c;
    for(int i=0; i<checkImageHeight; i++){
        for(int j=0; j<checkImageWidth; j++){
            c=((((i&0x8)==0)^((j&0x8))==0))*255;
            checkImage[i][j][0]=(GLubyte) c;
            checkImage[i][j][1]=(GLubyte) c;
            checkImage[i][j][2]=(GLubyte) c;
            checkImage[i][j][3]=(GLubyte) 255;
        }
    }
}

void init(void){
    glClearColor(0.0,0.0,0.0,0.0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);

    makeCheckImage();
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    glGenTextures(1,&texName);
    glBindTexture(GL_TEXTURE_2D,texName);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,checkImageWidth,
    checkImageHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,checkImage);
}

void display(void){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    glBindTexture(GL_TEXTURE_2D,texName);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-2.0, -1.0, 0.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-2.0, 1.0, 0.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(0.0, 1.0, 0.0);
    glTexCoord2f(1.0, 0.0); glVertex3f(0.0, -1.0, 0.0);
    
    glTexCoord2f(0.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(1.0, 1.0, 0.0);
    glTexCoord2f(1.0, 1.0); glVertex3f(2.41421, 1.0, -1.41421);
    glTexCoord2f(1.0, 0.0); glVertex3f(2.41421, -1.0, -1.41421);
    glEnd();
    glFlush();
    glDisable(GL_TEXTURE_2D);
}

void reshape(int w,int h){
    glViewport(0,0,(GLsizei) w,(GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0,(GLfloat) w/(GLfloat) h,1.0,30.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0,0.0,-3.6);
}

void keyboard(unsigned char key,int x,int y){
    switch (key) {
    case 27:
        exit(0);
        break;
    default:
        break;
    }
}

//Ogl section end


void load(char *filename){
    FILE *fp=fopen(filename,"rb");
    if(fp==NULL){
        printf("Nie mozna otworzyc!\n");
        return;
    }
    fseek(fp,0L,SEEK_END);
    int long size=ftell(fp);
    fsize=size;
    char *data=(char *) malloc(sizeof(char)*size);
    fseek(fp,0L,SEEK_SET);
    fread(data,sizeof(char),size,fp);
    for(int i=0; i<size; i++){
        memory[512+i]=data[i];
    }
    fclose(fp);
}

void loadFont(){
    for(int i=0x000; i<80; i++){
        memory[i]=font[i];
    }
}

void emulate(){
    unsigned short op=(memory[PC]<<8)|memory[PC+1];
    unsigned short target=0; unsigned short VX=0; unsigned short VY=0; unsigned short result=0;
    printf("ADDRES:0x%X\n",PC);
    switch(op&0xF000){
        case 0x0000:
            switch(op&0x000F){
                case 0x0000:
                    printf("Clear screen\n");
                    PC+=2;
                    break;
                case 0x000E:
                    printf("Return\n");
                    PC+=2;
                    break;
                default:
                    PC+=2;
                    printf("Nie znana instrukcja 0x0000! 0x%X\n",op&0x000F);
            }
            break;
        case 0x1000:
            printf("1NNN jump!\n");
            target=op&0x0FFF;
            printf("Jumped at 0x%X\n",target);
            PC=target;
            jumpPerformed=true;
            break;
        case 0x2000:
            printf("2NNN jump!\n");
            SP++;
            stack[SP]=PC;
            PC=(op&0x0FFF);
        case 0x3000:
            printf("3NNN jump!\n");
            if(V[memory[PC]&0x0F]==memory[PC+1]){
                PC+=4;
            } 
            PC+=2;
            break;
        case 0x4000:
            printf("4NNN jump!\n");
            if(V[memory[PC]&0x0F]!=memory[PC+1]){
                PC+=4;
            }
            PC+=2;
            break;
        case 0x5000:
            printf("5XY0 jump!\n");
            if(V[memory[PC]&0x0F]==V[(memory[PC+1]&0xF0)>>4]){
                PC+=4;
            }
            PC+=2;
            break;
        case 0x6000:
            printf("6XNN set register VX\n");
            target=op&0x00FF;
            V[(memory[PC]&0x0F)]=target;
            printf("6XNN I val:0x%X\n",V[(memory[PC]&0x0F)]);
            PC+=2;
            break;
        case 0x7000:
            printf("7XNN add value to register vx\n");
            target=op&0x00FF;
            V[(memory[PC]&0x0F)]+=target;
            printf("7XNN register:%d val:0x%X\n",memory[PC]&0x0F,V[(memory[PC]&0x0F)]);
            PC+=2;
            break;
        case 0x8000:
            switch(op&0x000F){
                case 0x0000:
                    printf("8XY0 Vx=Vy\n");
                    V[(memory[PC]&0x0F)]=V[(memory[PC+1]&0xF0)>>4];
                    PC+=2;
                    break;
                case 0x0001:
                    printf("8XY1 Vx|=Vy\n");
                    VX=memory[PC]&0x0F;
                    VY=((memory[PC+1]&0xF0)>>4);
                    result=VX|VY;
                    V[memory[PC]&0x0F]=result;
                    PC+=2;
                    break;
                case 0x0002:
                    printf("8XY2 Vx&=Vy\n");
                    VX=memory[PC]&0x0F;
                    VY=((memory[PC+1]&0xF0)>>4);
                    result=VX&VY;
                    V[memory[PC]&0x0F]=result;
                    PC+=2;
                    break;
                case 0x0003:
                    printf("8XY3 Vx^=Vy\n");
                    VX=memory[PC]&0x0F;
                    VY=((memory[PC+1]&0xF0)>>4);
                    result=VX^VY;
                    V[memory[PC]&0x0F]=result;
                    PC+=2;
                    break;
                case 0x0004:
                    printf("8XY4 Vx+=Vy\n");
                    if((memory[PC]&0x0F)>((memory[PC+1]&0xF0)>>4)){
                        V[15]=1;
                    } else {
                        V[15]=0;
                    }
                    V[memory[PC]&0x0F]+=V[(memory[PC+1]&0xF0)>>4];
                    PC+=2;
                    break;
                case 0x0005:
                    printf("8XY5 Vx-=Vy\n");
                    if((memory[PC]&0x0F)>((memory[PC+1]&0xF0)>>4)){
                        V[15]=0;
                    } else {
                        V[15]=1;
                    }
                    V[memory[PC]&0x0F]-=V[((memory[PC+1]&0xF0)>>4)];
                    PC+=2;
                    break;
                case 0x0006:
                    printf("8XY6 Vx>>=Vy\n");
                    V[15]=V[memory[PC]&0x0F]&0x1;
                    V[memory[PC]&0x0F]>>=1;
                    PC+=2;
                    break;
                case 0x0007:
                    printf("8XY7 Vx=Vy-Vx\n");
                    if((memory[PC]&0x0F)>((memory[PC+1]&0xF0)>>4)){
                        V[15]=0;
                    } else {
                        V[15]=1;
                    }
                    V[memory[PC]&0x0F]=V[(memory[PC+1]&0xF0)>>4]-V[memory[PC]&0x0F];
                    PC+=2;
                    break;
                case 0x000E:
                    printf("8XYE Vx<<=Vy\n");
                    V[15]=V[memory[PC]&0x0F]>>7;
                    V[memory[PC]&0x0F]<<=1;
                    PC+=2;
                    break;
                default:
                    printf("Unkown 0x8000 instruction\n");
                    PC+=2;
                    break;
            }
        case 0x9000:
            printf("9XY0 Vx!=Vy\n");
            if(V[memory[PC]&0x0F]>V[(memory[PC]&0xF0)>>4]){
                PC+=4;
            }
            PC+=2;
            break;
        case 0xA000:
            printf("ANN set index register I\n");
            target=op&0x0FFF;
            I=target;
            printf("ANN I val:0x%X\n",I);
            PC+=2;
            break;
        case 0xB000:
            printf("BNNN Vx!=Vy\n");
            target=op&0x0FFF;
            PC+=V[0]+target;
            break;
        case 0xC000:
            printf("CXNNN rand doesnt work now!\n");
            PC+=2;
            break;
        case 0xD000:
            printf("DXYN display/draw\n");
            short X=(op&0x0F00)>>8;
            //(memory[PC]&0x0F);
            short Y=(op&0x00F0)>>4;
            //(memory[PC+1]&0xF0)>>4;
            short Height=(op&0x000F);
            //(memory[PC+1]&0x0F);
            printf("Paint at:%d,%d height:%d\n",X,Y,Height);
            PC+=2;
            break;
        case 0xE000:
            switch(op&0x00FF){
                case 0x009E:
                    printf("Ex9E not implemented\n");
                    break;
                case 0x00A1:
                    printf("ExA1 not implemented\n");
                    break;
                default:
                    printf("Unkown 0xE000 instruction\n");
                    break;
            }
        case 0xF000:
            switch(op&0x00FF){
                case 0x000A:
                    printf("Instruction Fx0A not implemented\n");
                    break;
                case 0x0015:
                    printf("Instruction Fx15 not implemented\n");
                    break;
                case 0x0018:
                    printf("Instruction Fx18 not implemented\n");
                    break;
                case 0x001E:
                    I=I+V[memory[PC]&0x0F];
                    PC+=2;
                    break;
                case 0x0029:
                    printf("Instruction not implemented\n");
                    break;
                case 0x0033:
                    printf("FX33 BCD\n");
                    memory[I]=V[memory[PC]&0x0F]/100;
                    memory[I+1]=(V[memory[PC]&0x0F]/10)%10;
                    memory[I+2]=(V[memory[PC]&0x0F]%100)%10;
                    PC+=2;
                    break;
                case 0x0055:
                    printf("FX55 MEM\n");
                    for(int i=0; i<memory[PC]&0x0F; i++){
                        memory[I+i]=V[i];
                    }
                    I=I+memory[PC]&0x0F;
                    PC+=2;
                    break;
                case 0x0065:
                    printf("FX65 MEM\n");
                    for(int i=0; i<memory[PC]&0x0F; i++){
                        V[i]=memory[I+i];
                    }
                    I=I+memory[PC]&0x0F;
                    break;
                default:
                    printf("Unkown: 0xF000 instruction\n");
                    PC+=2;
                    break;
            }
        default:
            printf("Unkown: 0x%X\n",memory[PC]);
            PC+=2;
            break;
    }
}



int main(int argc,char *argv[]){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(250,250);
    glutCreateWindow("Chip8");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    /*load("chip8logo.ch8");
    printf("0x%X\n",memory[0x200]);
    for(int i=0; i<fsize/2; i++){
        emulate();
    }*/
    return 0;
}