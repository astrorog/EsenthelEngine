/******************************************************************************/
#include "!Header.h"
#include "!Set HP.h"
BUFFER(LocalFog)
   VecH LocalFogColor;
   Flt  LocalFogDensity;
   Vec  LocalFogInside;
BUFFER_END
#include "!Set LP.h"
/******************************************************************************/
// TODO: optimize fog shaders
void FogBox_VS(VtxInput vtx,
           out Vec4    outVtx :POSITION ,
           out Vec     outPos :TEXCOORD0,
           out Vec     outTex :TEXCOORD1,
           out Vec4    outSize:TEXCOORD2,
           out Matrix3 outMat :TEXCOORD3)
{
   outMat[0]=Normalize(MatrixX(ViewMatrix[0])); outSize.x=Length(MatrixX(ViewMatrix[0]));
   outMat[1]=Normalize(MatrixY(ViewMatrix[0])); outSize.y=Length(MatrixY(ViewMatrix[0]));
   outMat[2]=Normalize(MatrixZ(ViewMatrix[0])); outSize.z=Length(MatrixZ(ViewMatrix[0]));
                                                outSize.w=Max(outSize.xyz);

   // convert to texture space (0..1)
   outTex=vtx.pos()*0.5+0.5;
   outVtx=Project(outPos=TransformPos(vtx.pos()));
}
void FogBox_PS
(
   PIXEL,

   Vec     inPos :TEXCOORD0,
   Vec     inTex :TEXCOORD1,
   Vec4    inSize:TEXCOORD2,
   Matrix3 inMat :TEXCOORD3,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1,

   uniform Bool height
)
{
   Flt z  =TexDepthPoint(PixelToScreen(pixel));
   Vec pos=inTex,
       dir=Normalize(inPos); dir*=Min((SQRT3*2)*inSize.w, (z-inPos.z)/dir.z);
       dir=TransformTP(dir, inMat); // convert to box space

   // convert to texture space (0..1)
   dir=dir/(2*inSize.xyz);

   Vec end=pos+dir;

   if(end.x<0)end+=(0-end.x)/dir.x*dir;
   if(end.x>1)end+=(1-end.x)/dir.x*dir;
   if(end.y<0)end+=(0-end.y)/dir.y*dir;
   if(end.y>1)end+=(1-end.y)/dir.y*dir;
   if(end.z<0)end+=(0-end.z)/dir.z*dir;
   if(end.z>1)end+=(1-end.z)/dir.z*dir;

       dir =end-pos;
       dir*=inSize.xyz;
   Flt len =Length(dir)/inSize.w;

   Flt dns=LocalFogDensity;
   if(height){dns*=1-Avg(pos.y, end.y); len*=3;}

   color.rgb=LocalFogColor;
   color.a  =AccumulatedDensity(dns, len);
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
void FogBoxI_VS(VtxInput vtx,
            out Vec2    outTex  :TEXCOORD0,
            out Vec2    outPosXY:TEXCOORD1,
            out Vec4    outSize :TEXCOORD2,
            out Matrix3 outMat  :TEXCOORD3,
            out Vec4    outVtx  :POSITION )
{
   outMat[0]=Normalize(MatrixX(ViewMatrix[0])); outSize.x=Length(MatrixX(ViewMatrix[0]));
   outMat[1]=Normalize(MatrixY(ViewMatrix[0])); outSize.y=Length(MatrixY(ViewMatrix[0]));
   outMat[2]=Normalize(MatrixZ(ViewMatrix[0])); outSize.z=Length(MatrixZ(ViewMatrix[0]));
                                                outSize.w=Max(outSize.xyz);

   outVtx  =Vec4(vtx.pos2(), !REVERSE_DEPTH, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
   outTex  =vtx.tex();
   outPosXY=ScreenToPosXY(outTex);
}
void FogBoxI_PS
(
   NOPERSP Vec2    inTex  :TEXCOORD0,
   NOPERSP Vec2    inPosXY:TEXCOORD1,
   NOPERSP Vec4    inSize :TEXCOORD2,
   NOPERSP Matrix3 inMat  :TEXCOORD3,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1,

   uniform Int  inside,
   uniform Bool height
)
{
   Vec pos=GetPosPoint(inTex, inPosXY),
       dir=Normalize(pos); dir*=Min((SQRT3*2)*inSize.w, (pos.z-Viewport.from)/dir.z);
       dir=TransformTP(dir, inMat); // convert to box space

   // convert to texture space (0..1)
   pos=LocalFogInside/(2*inSize.xyz)+0.5;
   dir=dir           /(2*inSize.xyz);

   if(inside==0)
   {
      if(pos.x<0)pos+=(0-pos.x)/dir.x*dir;
      if(pos.x>1)pos+=(1-pos.x)/dir.x*dir;
      if(pos.y<0)pos+=(0-pos.y)/dir.y*dir;
      if(pos.y>1)pos+=(1-pos.y)/dir.y*dir;
      if(pos.z<0)pos+=(0-pos.z)/dir.z*dir;
      if(pos.z>1)pos+=(1-pos.z)/dir.z*dir;
   }

   Vec end=pos+dir;

   if(end.x<0)end+=(0-end.x)/dir.x*dir;
   if(end.x>1)end+=(1-end.x)/dir.x*dir;
   if(end.y<0)end+=(0-end.y)/dir.y*dir;
   if(end.y>1)end+=(1-end.y)/dir.y*dir;
   if(end.z<0)end+=(0-end.z)/dir.z*dir;
   if(end.z>1)end+=(1-end.z)/dir.z*dir;

       dir =end-pos;
       dir*=inSize.xyz;
   Flt len =Length(dir)/inSize.w;

   Flt dns=LocalFogDensity;
   if(height){dns*=1-Avg(pos.y, end.y); len*=3;}

   color.rgb=LocalFogColor;
   color.a  =AccumulatedDensity(dns, len);
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
void FogBall_VS(VtxInput vtx,
            out Vec4 outVtx :POSITION ,
            out Vec  outPos :TEXCOORD0,
            out Vec  outTex :TEXCOORD1,
            out Flt  outSize:TEXCOORD2)
{
   outTex =vtx.pos();
   outSize=Length(MatrixX(ViewMatrix[0]));
   outVtx =Project(outPos=TransformPos(vtx.pos()));
}
void FogBall_PS
(
   PIXEL,

   Vec inPos :TEXCOORD0,
   Vec inTex :TEXCOORD1,
   Flt inSize:TEXCOORD2,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Flt z  =TexDepthPoint(PixelToScreen(pixel));
   Vec pos=Normalize    (inTex),
       dir=Normalize    (inPos); Flt max_length=(z-inPos.z)/(dir.z*inSize);
       dir=Transform3   (dir, CamMatrix); // convert to ball space

   // collision detection
   Vec p  =PointOnPlane(pos, dir);
   Flt s  =Length      (p       );
       s  =Sat         (1-s*s   );
   Vec end=p+Sqrt(s)*dir;

   Flt len=Min(Dist(pos, end), max_length);

   Flt dns=LocalFogDensity*s;

   color.rgb=LocalFogColor;
   color.a  =AccumulatedDensity(dns, len);
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
void FogBallI_VS(VtxInput vtx,
             out Vec2 outTex  :TEXCOORD0,
             out Vec2 outPosXY:TEXCOORD1,
             out Flt  outSize :TEXCOORD2,
             out Vec4 outVtx  :POSITION )
{
   outVtx  =Vec4(vtx.pos2(), !REVERSE_DEPTH, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
   outTex  =vtx.tex();
   outPosXY=ScreenToPosXY(outTex);
   outSize =Length(MatrixX(ViewMatrix[0]));
}
void FogBallI_PS
(
   NOPERSP Vec2 inTex  :TEXCOORD0,
   NOPERSP Vec2 inPosXY:TEXCOORD1,
   NOPERSP Flt  inSize :TEXCOORD2,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1,

   uniform Int inside
)
{
   Vec pos=GetPosPoint(inTex, inPosXY),
       dir=Normalize (pos); Flt max_length=(pos.z-Viewport.from)/(dir.z*inSize);
       dir=Transform3(dir, CamMatrix); // convert to ball space

   pos=LocalFogInside/inSize;

   // collision detection
   Vec p  =PointOnPlane(pos, dir);
   Flt s  =Length      (p       );
       s  =Sat         (1-s*s   );
   Vec end=p+dir*Sqrt(s);

   Flt len=Min(Dist(pos, end), max_length);

   Flt dns=LocalFogDensity*s;

   color.rgb=LocalFogColor;
   color.a  =AccumulatedDensity(dns, len);
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
TECHNIQUE(FogBox ,  FogBox_VS(),  FogBox_PS(   false));
TECHNIQUE(FogBox0, FogBoxI_VS(), FogBoxI_PS(0, false));
TECHNIQUE(FogBox1, FogBoxI_VS(), FogBoxI_PS(1, false));

TECHNIQUE(FogHgt ,  FogBox_VS(),  FogBox_PS(   true));
TECHNIQUE(FogHgt0, FogBoxI_VS(), FogBoxI_PS(0, true));
TECHNIQUE(FogHgt1, FogBoxI_VS(), FogBoxI_PS(1, true));

TECHNIQUE(FogBall ,  FogBall_VS(),  FogBall_PS( ));
TECHNIQUE(FogBall0, FogBallI_VS(), FogBallI_PS(0));
TECHNIQUE(FogBall1, FogBallI_VS(), FogBallI_PS(1));
/******************************************************************************/
