
Texture2D gInput : register(t0);
Texture2D gSobelResult : register(t1);
RWTexture2D<float4> gOutput : register(u0);

float CalcLuminance(float3 color) {
  return dot(color, float3(0.299F, 0.587F, 0.114F));
}

[numthreads(32, 32, 1)] 
void SobelMain(int3 dispatchThreadID
                                       : SV_DispatchThreadID) {
  float4 c[3][3];

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      int2 xy = dispatchThreadID.xy + int2(-1 + j, -1 + i);
      c[i][j] = gInput[xy];
    }

  float4 dx = -1.0F * c[0][0] - 2.0F * c[1][0] - 1.0F * c[2][0] +
              1.0F * c[0][2] + 2.0F * c[1][2] + 1.0F * c[2][2];

  float4 dy = -1.0F * c[2][0] - 2.0F * c[2][1] - 1.0F * c[2][2] +
              1.0F * c[0][0] + 2.0F * c[0][1] + 1.0F * c[0][2];

  float4 dxdy = sqrt(dx * dx + dy * dy);

  dxdy = 1.0F - saturate(CalcLuminance(dxdy.rgb));
  gOutput[dispatchThreadID.xy] = dxdy;
}
