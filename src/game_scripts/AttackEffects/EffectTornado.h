using UnityEngine;

public class TornadoScript : MonoBehaviour
{
    public float radius = 1;
    public float speed = 5;
    private string name = "Tornado";
    public float tornadoRemainingTime = 5;
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
        //Mo�na doda� sk�adnik 1 �eby podnie�� �rednic� tornada 
        if (special1)
        {
            radius *= modifier;
        }
        //Mo�na doda� sk�adnik 2 �eby zwi�kszy� czas trwania tornada 
        if (special2)
        {
            tornadoRemainingTime *= modifier;
        }

        EventManager.Emit(new PotionExplodeEvent()
        {
            position = this.transform.position,
            radius = this.radius,
            name = this.name,
            effectTime = tornadoRemainingTime,
            ingredientCount = this.ingredientCount,
            special1 = this.special1,
            special2 = this.special2,
            damage = this.damage,
            timeInterval = this.timeInterval
        });


    }

    private void OnCollisionEnter(Collision collision)
    {
        if(!collision.collider.CompareTag("EnemyBullet"))
        {
            return;
        }
        if (ingredientCount == 1)
        {
            //1 sk�adnik:  despawnuje pociski wrog�w lec�ce przez nie. 
           // if (collision.collider.CompareTag("EnemyBullet"))
          //  {
                Destroy(collision.collider.gameObject);
            //}
        }
        else
        {
            //2 sk�adniki:  przechwytuje pociski wrog�w lec�ce przez nie. Pociski kr�c� si� po obwodzie tornada i mog� trafi� innych przeciwnik�w
            EnemyBullet bullet = collision.collider.GetComponent<EnemyBullet>();
            if (bullet != null)
            {
                bullet.BulletInTornadoAction(this.transform, radius, rotationSpeed);
            }
        }
    }
    public float rotationSpeed = 90f;
    public void Update()
    {

        float factor = lifetime / radius;
        factor = Mathf.Sin(factor * Mathf.PI / 2);
        explosionRenderer.material.SetFloat("_ExplosionTime", factor * radius);

        //Tornado kr�ci si�
        transform.Rotate(Vector3.up, rotationSpeed * Time.deltaTime);


        lifetime += Time.deltaTime;

        //if (lifetime > radius)
        //{
        //    Destroy(this.gameObject);
        //}
        if ( lifetime>tornadoRemainingTime)
        {
            Destroy(this.gameObject);
        }
    }
}
