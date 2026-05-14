using UnityEngine;

public class PetrifyScript : MonoBehaviour
{
    public float radius = 1;
    public float speed = 5;
    private string name = "Petrify";
    public float petrifyRemainingTime = 5;
    public int ingredientCount = 1;
    public bool special1 = false;
    public bool special2 = false;
    public MeshRenderer explosionRenderer;
    private float lifetime;
    public int modifier = 2;

    public void Awake()
    {
        //Mo�na doda� sk�adnik 1 �eby podnie�� czas zamro�enia 
        if (special1)
        {
            petrifyRemainingTime *= modifier;
        }
        //Mo�na doda� sk�adnik 2 �eby podnie�� zasi�g AoE
        if (special2)
        {
            radius *= modifier;
        }

        EventManager.Emit(new PotionExplodeEvent()
        {
            position = this.transform.position,
            radius = this.radius,
            name = this.name,
            effectTime = petrifyRemainingTime,
            ingredientCount = this.ingredientCount,
            special1 = this.special1,
            special2 = this.special2,
            damage = 0,
            timeInterval = 0
        });


    }

    public void Update()
    {
        
            float factor = lifetime / radius;
            factor = Mathf.Sin(factor * Mathf.PI / 2);
            explosionRenderer.material.SetFloat("_ExplosionTime", factor * radius);
           
        
        lifetime += Time.deltaTime * speed;

        if (lifetime > radius)
        {
            Destroy(this.gameObject); 
        }
    }
}
