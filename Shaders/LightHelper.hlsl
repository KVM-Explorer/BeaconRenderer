
struct Material {
	float4 Diffuse;
	float3 FresnelR0;
	float Roughness;
};

struct Light {
	float3 Position;
	float LightBegin;
	float3 LightStrengh;
	float LightEnd;
};

float LinearAttenuation(float p, float begin, float end) {
	return (end - p) / (end - begin);
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec) {
	float cosIncidentAngle = saturate(dot(normal, lightVec));
	float f0 = 1.0 - cosIncidentAngle;
	float3 reflectPercent = R0 + (1 - R0) * (f0 * f0 * f0 * f0 * f0);
	return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal,
									float3 toEye, Material material) {

	float shininess = 1 - material.Roughness;
	const float m = shininess * 256.0F;
	float3 halfVec = normalize(toEye + lightVec);

	float roughnessFactor =
			(m + 8.0F) * pow(max(dot(halfVec, normal), 0.0F), m) / 8.0F;
	float3 fresnelFactor = SchlickFresnel(material.FresnelR0, halfVec, lightVec);

	float3 specAlbedo = fresnelFactor * roughnessFactor;
	specAlbedo = specAlbedo / (specAlbedo + 1.0F);

	return (material.Diffuse.rgb + specAlbedo) * lightStrength;
}

float3 PointLightColor(Light light, Material material, float3 pos,
											 float3 normal, float3 toEye) {
	float3 lightVec = light.Position - pos;
	float distance = length(lightVec);

	if (distance < light.LightBegin || distance > light.LightEnd)
		return 0.0F;

	lightVec = lightVec / distance;

	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = light.LightStrengh * ndotl;

	// Attenuate light by distance.
	float att = LinearAttenuation(distance, light.LightBegin, light.LightEnd);
	lightStrength *= att;
	return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}