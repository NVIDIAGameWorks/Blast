#include "common_buffers.hlsl"

static const float att_c = 1.0f;
static const float att_l = 0.014f;
static const float att_q = 0.0007f;


float CalcAttenuation(float distance)
{
	return 1 / (att_c + att_l * distance + att_q * distance * distance);
};


float3 CalcLight(float3 textureColor, float3 lightDir, float3 viewDir, float3 normal, float3 lightColor, float specPower, float specIntensity, float attenuation)
{
	normal = normalize(normal);

	// diffuse
	float3 dirToLight = normalize(-lightDir);
	float diffuseFactor = max(dot(normal, dirToLight), 0.0);
	float3 diffuse = lightColor * textureColor * diffuseFactor * attenuation;

	// specular (Blinn-Phong)
	float3 halfwayDir = normalize(dirToLight + viewDir);
	float specFactor = pow(max(dot(viewDir, halfwayDir), 0.0), specPower);
	float3 spec = lightColor * specFactor * attenuation * specIntensity;
	
	return diffuse + spec;
};

float3 CalcPixelLight(float3 diffuseColor, float3 worldPos, float3 normal)
{
	float3 viewDir = normalize(viewPos - worldPos);

	// ambient
	float3 ambient = ambientColor * diffuseColor;

	// dir light
	float3 dirLight = CalcLight(diffuseColor, dirLightDir, viewDir, normal, dirLightColor, specularPower, specularIntensity, 1);

	// point light
	float3 pointLightDir = worldPos - pointLightPos;
	float distance = length(pointLightDir);
    float attenuation = CalcAttenuation(distance);
	float3 pointLight = CalcLight(diffuseColor, pointLightDir, viewDir, normal, pointLightColor, specularPower, specularIntensity, attenuation);
	
	// hacky hack: ambient attenuates within point light distance
	ambient *= attenuation;

	return ambient + dirLight + pointLight;
};