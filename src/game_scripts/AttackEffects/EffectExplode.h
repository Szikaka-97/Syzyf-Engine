using UnityEngine;

public class ExplosionEffect2 : MonoBehaviour
{
    // Wartoœæ od 0 do 1 w zale¿noœci od proporcji sk³adniku eksplozji w potce
    [Range(0, 1)]
    public float strength;
    public float maxRange;
    public float maxDamage;
    private float lifetime;
    public MeshRenderer explosionRenderer;

    private float GetRange() {
        return strength * maxRange;
    }

    private float GetDamage() {
        return strength * maxDamage;
    }

    public void Awake() {
        this.explosionRenderer.transform.localScale = Vector3.one * GetRange();

		foreach (var enemy in GameObject.FindObjectsByType<Skeleton>(FindObjectsSortMode.None)) {
            if (Vector3.Distance(transform.position, enemy.transform.position) <= GetRange()) {
                enemy.TakeDamage(GetDamage());

				if (enemy.IsDead()) {
					foreach (var body in enemy.GetComponentsInChildren<Rigidbody>(true))
					{
						body.AddExplosionForce(10, transform.position, GetRange() * 2, 1, ForceMode.Impulse);
					}
				}
            }
        }
    }

    public void Update() {
        float factor = lifetime / GetRange();

        factor = Mathf.Sin(factor * Mathf.PI / 2);

        explosionRenderer.material.SetFloat("_ExplosionTime", factor * GetRange());

        lifetime += Time.deltaTime * 30;

        if (lifetime > GetRange()) {
            Destroy(this.gameObject);
        }
    }
}
