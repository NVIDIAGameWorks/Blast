using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Demo : MonoBehaviour
{
    public GameObject hitSphere;

	void Awake ()
    {
		//NativeLoader.Load();

		generateCity();
    }

    private void generateCity()
    {
        const int BUILDING_TYPE_COUNT = 5;
        Vector3 BUILDING_MIN_SIZE = new Vector3(10, 10, 10);
        Vector3 BUILDING_MAX_SIZE = new Vector3(50, 200, 50);

        List<CubeAsset> buildingTypes = new List<CubeAsset>(BUILDING_TYPE_COUNT);
        for (int i = 0; i < BUILDING_TYPE_COUNT; ++i)
        {
            CubeAsset.Settings settings = new CubeAsset.Settings();
            settings.depths.Add(new CubeAsset.DepthInfo(new Vector3(1, 1, 1), NvBlastChunkDesc.Flags.NoFlags));
            settings.depths.Add(new CubeAsset.DepthInfo(new Vector3(1, 2, 1), NvBlastChunkDesc.Flags.NoFlags));
            settings.depths.Add(new CubeAsset.DepthInfo(new Vector3(2, 3, 2), NvBlastChunkDesc.Flags.NoFlags));
            settings.depths.Add(new CubeAsset.DepthInfo(new Vector3(2, 2, 2), NvBlastChunkDesc.Flags.SupportFlag));
            settings.extents = new Vector3(Random.Range(BUILDING_MIN_SIZE.x, BUILDING_MAX_SIZE.x), Random.Range(BUILDING_MIN_SIZE.y, BUILDING_MAX_SIZE.y), Random.Range(BUILDING_MIN_SIZE.z, BUILDING_MAX_SIZE.z));
            settings.staticHeight = 10.0f;

            CubeAsset cubeAsset = CubeAsset.generate(settings);

            buildingTypes.Add(cubeAsset);
        }

        int totalBuildings = 0;

        const float CITY_HALF_SIZE = 200.0f;
        const float SPARSITY = 30.0f;
        const int BUILDING_COUNT_MAX = 20;
        Vector2 pos = new Vector2(-CITY_HALF_SIZE, -CITY_HALF_SIZE);
        float buildingYMax = 0.0f;
        while (pos.y < CITY_HALF_SIZE && totalBuildings < BUILDING_COUNT_MAX)
        {
            // random jump
            pos.x += Random.Range(0.0f, SPARSITY);
            if(pos.x > CITY_HALF_SIZE)
            {
                pos.x = -CITY_HALF_SIZE;
                pos.y += buildingYMax + Random.Range(0.0f, SPARSITY);
                buildingYMax = 0.0f;
                continue;
            }

            // random bulding type spawn
            int type = Random.Range(0, buildingTypes.Count);
            var cubeFamily = (new GameObject("Cube Actor #" + type)).AddComponent<CubeFamily>();
            CubeAsset asset = buildingTypes[type];
            cubeFamily.transform.localPosition = new Vector3(pos.x, asset.extents.y / 2.0f, pos.y);
            pos.x += asset.extents.x;
            buildingYMax = Mathf.Max(buildingYMax, asset.extents.z);
            cubeFamily.Initialize(asset);
            totalBuildings++;
        }
    }

    private IEnumerator applyRadialDamage(Vector3 position, float minRadius, float maxRadius, float compressive, float explosive = 3000.0f)
    {
        var hits = Physics.OverlapSphere(position, maxRadius);
        foreach (var hit in hits)
        {
            var rb = hit.GetComponentInParent<Rigidbody>();
            var family = hit.GetComponentInParent<CubeFamily>();
            if (rb != null && family != null)
            {
                family.ApplyRadialDamage(rb, position, minRadius, maxRadius, compressive);
            }
        }

        yield return new WaitForEndOfFrame();

        hits = Physics.OverlapSphere(position, maxRadius);
        foreach (var hit in hits)
        {
            var rb = hit.GetComponentInParent<Rigidbody>();
            if(rb != null)
            {
                rb.AddExplosionForce(explosive, position, maxRadius, 3.0f);
            }
        }
    }

    private void Update()
    {
        hitSphere.SetActive(false);
        bool isActive = false;
        if (true)
        {
            var ray = Camera.main.ScreenPointToRay(Input.mousePosition);
            RaycastHit hit;
            if(Physics.Raycast(ray, out hit))
            {
                hitSphere.transform.position = hit.point;
                isActive = true;
            }
        }

        _hitSphereSize += Input.GetAxis("Mouse ScrollWheel") * 10.0f;

        if (Input.GetMouseButton(0))
        {
            StartCoroutine(applyRadialDamage(hitSphere.transform.position, 0.0f, _hitSphereSize, 10.0f));
        }

        hitSphere.SetActive(isActive);
        hitSphere.transform.localScale = new Vector3(_hitSphereSize, _hitSphereSize, _hitSphereSize);
    }

    private float _hitSphereSize = 25.0f;
}
