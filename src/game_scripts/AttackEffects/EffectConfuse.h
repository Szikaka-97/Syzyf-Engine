using UnityEngine;

public class ConfuseEffect : MonoBehaviour
{
    public float radius = 1;
    public float speed = 5;
    private string name = "Confuse";
    public float confuseRemainingTime = 5;
    public int ingredientCount = 1;
    public bool special1 = false;
    public bool special2 = false;
    //public MeshRenderer explosionRenderer;
    private float lifetime;
    public int modifier = 2;
    public int damage = 25;
    //public int timeInterval = 1;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        //Mo�na doda� sk�adnik 1 �eby podnie�� czas trwania efektu
        if(special1)
        {
            confuseRemainingTime *= modifier;
        }

        //Mo�na doda� sk�adnik 2 �eby zwi�kszy� czas zadawania obra�e�
        if (special2)
        {
            damage *= modifier;
        }

        EventManager.Emit(new PotionExplodeEvent()
        {
            position = this.transform.position,
            radius = this.radius,
            name = this.name,
            effectTime = confuseRemainingTime,
            ingredientCount = this.ingredientCount,
            special1 = this.special1,
            special2 = this.special2,
            damage = this.damage,
            timeInterval = 0
        });
    }

    // Update is called once per frame
    void Update()
    {
        lifetime += Time.deltaTime * speed;

        if (lifetime > radius)
        {
            Destroy(this.gameObject);
        }
    }
}
