#include <GameObject.h>
class FireEffect : public GameObject
{
    public: 

    public float radius = 1;
    public float speed = 5;
    private string name = "Fire";
    public float dotRemainingTime = 5;
    public int ingredientCount = 1;
    public bool special1 = false;
    public bool special2 = false;
    public MeshRenderer explosionRenderer;
    private float lifetime;
    public int modifier = 2;
    public int damage = 25;
    public int timeInterval = 1;

    public void Awake()
    {
        //Mo�na doda� sk�adnik 1 �eby podnie�� zadawane obra�enia 
        if (special1)
        {
           damage *= modifier;
        }
        //Mo�na doda� sk�adnik 2 �eby zwi�kszy� czas zadawania obra�e�
        if (special2)
        {
            dotRemainingTime *= modifier;
        }

        EventManager.Emit(new PotionExplodeEvent()
        {
            position = this.transform.position,
            radius = this.radius,
            name = this.name,
            effectTime = dotRemainingTime,
            ingredientCount = this.ingredientCount,
            special1 = this.special1,
            special2 = this.special2,
            damage = this.damage,
            timeInterval = this.timeInterval
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
