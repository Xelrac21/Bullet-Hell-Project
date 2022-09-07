#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <math.h>
#include <string>

#define PI 3.14159265358979323846264338327950288419716939937510
#define FPS			60.0f
#define TIMESTEP	(1.0f / FPS)
#define shotPoolSize 4000
#define number_of_behaviors 6
#define force 10.0f

const float screen_w = 800;
const float screen_h = 1000;

sf::RenderWindow window(sf::VideoMode(screen_w, screen_h), "U.Grid");
float frictionCoefficient = 0.2;
float elasticity = 1.0;

//const float box_row = 50;
//const float box_column = 60;
const float cell_w = 10; //change this to a value slight higher than your shot_w and shot_h if you change to a shot with a different size
const float cell_h = 10;
const int cell_rownum = (screen_h - 1) / cell_h + 1;
const int cell_colnum = (screen_w - 1) / cell_w + 1;
//const float box_w = screen_w / box_column;
//const float box_h = screen_h / 2 / box_row;
//const float player_w = 10;
//const float player_h = 10;
//const float player_s = 2;
const float shot_w = 8; //change this if change the shot to a different size. Don't use exact measurement. Dapat parehong number both sides
const float shot_h = 8;
//const float shot_s = 6;
bool Reset = true;
bool IsShooting = false;
bool AlternateShot1 = false;
bool AlternateShot2 = false;



struct Entity {
	int durability;
	float entityHeight;
	float entityWidth;
	sf::Vector2f velocity;
	sf::Vector2f acceleration;
	float speed;
	sf::Vector2f spawnPosition;
	float cooldown;
	float shotSpeed;
	sf::Clock timer;
	sf::Clock timer2;
	int behavior = 0;
	float Mass = 1.0f;

	bool isComingIn = false;
	bool isSprited = false;
	sf::Sprite entitySprite;
	sf::Texture texture;
};

struct entityShots {
	float shotHeight;
	float shotWidth;
	sf::Vector2f velocity;
	sf::Vector2f acceleration;
	int type;
	float speed;
	bool isSprited = false;
	sf::Sprite entityShot;
	sf::Texture texture;
	float Mass = 1.0;
	void update(float t)
	{
		entityShot.setPosition(entityShot.getPosition().x + t*velocity.x, entityShot.getPosition().y + t*velocity.y);
	}


};

//std::vector<entityShots> shotPoolReal(shotPoolSize);
std::vector<entityShots*> shotPool;
std::vector<entityShots*> shotsOnScreen;
std::vector<Entity*> enemies;
//std::vector<Entity> enemiesReal;

std::vector<std::vector<std::vector<entityShots*> > > uniform_grid;

float magnitude(sf::Vector2f a)
{
	return sqrt(a.x*a.x + a.y*a.y);
}

void normalize(sf::Vector2f &a)
{
	if (magnitude(a)) a /= magnitude(a);
}

void gridPlacement(entityShots *s, float shot_s) {
	/// All objects are circles soooooooooo rip in pieces
	/// Always contained by only 4 cells because cell side is half of biggest object
	sf::Vector2f pos = s->entityShot.getPosition();
	int minr = (pos.y - (shot_s / 2)) / cell_h;
	int maxr = (pos.y + (shot_s / 2)) / cell_h;
	int minc = (pos.x - (shot_s / 2)) / cell_w;
	int maxc = (pos.x + (shot_s / 2)) / cell_w;
	/// Check if out of bounds
	//std::cout << minrow << " | " << maxrow << "\n";
	//std::cout << mincol << " | " << maxcol << "\n";
	if (maxr > cell_rownum - 1)
	{
		maxr = cell_rownum - 1;
	}
	else
	{
		maxr = maxr;
	}
	if (maxc > cell_colnum - 1)
	{
		maxc = cell_colnum - 1;
	}
	else
	{
		maxc = maxc;
	}
	if (minr < 0)
	{
		minr = 0;
	}
	else
	{
		minr = minr;
	}
	if (minc < 0)
	{
		minc = 0;
	}
	else
	{
		minc = minc;
	}
	for (int i = minr; i <= maxr; i++) {
		for (int j = minc; j <= maxc; j++) {
			uniform_grid[i][j].push_back(s);
		}
	}
}
void clearGrid()
{
	for (int i = 0; i < cell_rownum; i++) {
		for (int j = 0; j < cell_rownum; j++) {
			uniform_grid[i][j].clear();
		}
	}
}

bool collisionHappen(entityShots *p, entityShots *b)
{
	sf::FloatRect rect1 = p->entityShot.getGlobalBounds();
	bool isColliding = rect1.intersects(b->entityShot.getGlobalBounds());

	sf::Vector2f distance(p->entityShot.getPosition().x - b->entityShot.getPosition().x, p->entityShot.getPosition().y - b->entityShot.getPosition().y);
	float distanceMagnitude = sqrt(distance.x*distance.x + distance.y*distance.y);

	sf::Vector2f collisionNormal(distance.x / distanceMagnitude, distance.y / distanceMagnitude);

	sf::Vector2f relativeVelocity(p->velocity.x - b->velocity.x, p->velocity.y - b->velocity.y);

	float Rmagnitude = sqrt(relativeVelocity.x*relativeVelocity.x + relativeVelocity.y*relativeVelocity.y);

	float RdotC = relativeVelocity.x*collisionNormal.x + relativeVelocity.y*collisionNormal.y;
	float CdotC = collisionNormal.x*collisionNormal.x + collisionNormal.y*collisionNormal.y;

	if (isColliding)
	{
		if (RdotC / Rmagnitude < 0.0)
		{
			float impulse = -((1 + elasticity)*(RdotC)) / ((CdotC)*(1 / p->Mass + 1 / b->Mass));
			p->velocity.x += impulse / p->Mass*collisionNormal.x;
			p->velocity.y += impulse / p->Mass*collisionNormal.y;
			b->velocity.x -= impulse / b->Mass*collisionNormal.x;
			b->velocity.y -= impulse / b->Mass*collisionNormal.y;
			return true;
		}
	}

	else
	{
		return false;
	}
}

void movement(Entity* entity) {
	//apply acceleration
	/*if (entity->acceleration.x != 0 || entity->acceleration.y != 0) {
		entity->velocity.x = entity->acceleration.x*TIMESTEP;
		entity->velocity.y = entity->acceleration.y*TIMESTEP;
	}*/

	//apply friction
	//entity->velocity.x += -frictionCoefficient*entity->velocity.x*TIMESTEP / entity->Mass;
	//entity->velocity.y += -frictionCoefficient*entity->velocity.y*TIMESTEP / entity->Mass;

	//apply wall boundaries if player or when enemies are going to spawn point
	if (!entity->isComingIn || entity->behavior == 0) {
		if (entity->entitySprite.getPosition().x - entity->entitySprite.getGlobalBounds().width / 2.0f <= 0) {
			entity->entitySprite.setPosition(entity->entitySprite.getGlobalBounds().width / 2.0f + 1.0f, entity->entitySprite.getPosition().y); entity->velocity.x = -elasticity*entity->velocity.x;
		}

		if (entity->entitySprite.getPosition().x + entity->entitySprite.getGlobalBounds().width / 2.0f >= screen_w) {
			entity->entitySprite.setPosition((screen_w - entity->entitySprite.getGlobalBounds().width / 2.0f - 1.0f), entity->entitySprite.getPosition().y); entity->velocity.x = -elasticity*entity->velocity.x;
		}

		if (entity->entitySprite.getPosition().y + entity->entitySprite.getGlobalBounds().height / 2.0f >= screen_h) {
			entity->entitySprite.setPosition(entity->entitySprite.getPosition().x, (screen_h - entity->entitySprite.getGlobalBounds().height / 2.0f) -1.0f); entity->velocity.y = -elasticity*entity->velocity.y;
		}

		if (entity->entitySprite.getPosition().y - entity->entitySprite.getGlobalBounds().height / 2.0f <= 0) {
			entity->entitySprite.setPosition(entity->entitySprite.getPosition().x, entity->entitySprite.getGlobalBounds().height / 2.0f + 1.0f); entity->velocity.y = -elasticity*entity->velocity.y;
		}
	}

	normalize(entity->velocity);

	entity->entitySprite.move(TIMESTEP*entity->speed*entity->velocity.x, TIMESTEP*entity->speed*entity->velocity.y);
}

void behaviorAssigner(Entity* entity, int num_behavior) {
	if (num_behavior == 0) {
		srand(rand() ^ time(0));
		entity->behavior = rand() % number_of_behaviors + 1;
	}
	else entity->behavior = num_behavior;
}

void enemySpawner(Entity* entity) {
	srand(rand() ^ time(0));

	entity->entityWidth = entity->texture.getSize().x;
	entity->entityHeight = entity->texture.getSize().y;

	entity->entitySprite.setOrigin(entity->entityWidth / 2.0f, entity->entityHeight / 2.0f);

	int counter = rand() % 3;
	if (counter == 0) entity->entitySprite.setPosition(-5.0 - entity->entityWidth, fmod(rand(), (0.5*screen_h)));
	else if (counter == 1) entity->entitySprite.setPosition(5.0 + screen_w + entity->entityWidth, fmod(rand(), 0.5*screen_h));
	else if (counter == 2) entity->entitySprite.setPosition(fmod(rand(), (screen_w + 100.0f)) - 50.0f, -5.0 - entity->entityHeight);

	entity->spawnPosition.x = fmod(rand(), (screen_w - entity->entityWidth)) + entity->entityWidth / 2.0f;
	entity->spawnPosition.y = fmod(rand(), (0.5*screen_h - entity->entityHeight)) + entity->entityWidth / 2.0f;
}

//enemy behavior where it spawns offscreen and moves to its intended location. Once there, it fires four shots with two at the front and two at an angled direction to the sides. It moves only along the x axis
//with a small chance to change direction
void enemyBehavior1(Entity* entity) {
	srand(rand() ^ time(0));

	//when recently spawned and outside of window, prime for coming in
	if (!entity->isSprited) {
		entity->texture.loadFromFile("data/Enemy2StillNEW.png");
        sf::Vector2u textureSize = entity->texture.getSize();
        entity->entitySprite.setTexture(entity->texture);
        entity->entityHeight = textureSize.y;
        entity->entityWidth = textureSize.x;
        entity->entitySprite.setOrigin(textureSize.x/2.0f, textureSize.y/2.0f);
        entity->isSprited = true;

        entity->shotSpeed = 6.0f;

        enemySpawner(entity);

        entity->entitySprite.setPosition((rand()%2)*window.getSize().x, entity->spawnPosition.y);
        entity->isComingIn = true;
	}

	//getting to position
	if (entity->isComingIn) {
		if (entity->entitySprite.getPosition().x < entity->spawnPosition.x) entity->velocity = sf::Vector2f(0.1f, 0.0f);
		else if (entity->entitySprite.getPosition().x > entity->spawnPosition.x) entity->velocity = sf::Vector2f(-1.0f, 0.0f);

		movement(entity);
	}

	//once in place, do its stuff
	if (entity->entitySprite.getPosition().x <= entity->spawnPosition.x + 2.0 &&
		entity->entitySprite.getPosition().x >= entity->spawnPosition.x - 2.0 &&
		entity->entitySprite.getPosition().y <= entity->spawnPosition.y + 2.0 &&
		entity->entitySprite.getPosition().y >= entity->spawnPosition.y - 2.0) entity->isComingIn = false;

	if (!entity->isComingIn) {
		if (rand() % 101 == 0) entity->velocity.x = -entity->velocity.x;
		movement(entity);

		if (shotPool.size() > 4 && entity->timer.getElapsedTime().asSeconds() > entity->cooldown) {
			entityShots *holder1 = shotPool.back();
			holder1->texture.loadFromFile("data/EnemyMissile1.png");
			holder1->entityShot.setTexture(holder1->texture);
			holder1->shotHeight = holder1->texture.getSize().y;
			holder1->shotWidth = holder1->texture.getSize().x;
			holder1->speed = entity->shotSpeed;
			holder1->velocity = sf::Vector2f(0, 0.1f);;
			holder1->type = 0;
			holder1->entityShot.setOrigin(holder1->entityShot.getLocalBounds().width / 2, holder1->entityShot.getLocalBounds().height / 2);
			holder1->entityShot.setPosition(entity->entitySprite.getPosition().x + entity->entityWidth / 4.0f, entity->entitySprite.getPosition().y + entity->entityHeight / 8.0f);
			shotsOnScreen.push_back(holder1); //Push back to the Vector
			shotPool.pop_back(); //Removes from the Shot Pool

			entityShots* holder2 = shotPool.back();
			holder2->texture.loadFromFile("data/EnemyMissile1.png");
			holder2->entityShot.setTexture(holder2->texture);
			holder2->shotHeight = holder2->texture.getSize().y;
			holder2->shotWidth = holder2->texture.getSize().x;
			holder2->speed = entity->shotSpeed;
			holder2->type = 0;
			holder2->velocity = sf::Vector2f(0, 0.1f);;
			holder2->entityShot.setOrigin(holder2->entityShot.getLocalBounds().width / 2, holder2->entityShot.getLocalBounds().height / 2);
			holder2->entityShot.setPosition(entity->entitySprite.getPosition().x - entity->entityWidth / 4.0f, entity->entitySprite.getPosition().y + entity->entityHeight / 8.0f);
			shotsOnScreen.push_back(holder2); //Push back to the Vector
			shotPool.pop_back(); //Removes from the Shot Pool

			entityShots* holder3 = shotPool.back();
			holder3->texture.loadFromFile("data/EnemyMissile1.png");
			holder3->entityShot.setTexture(holder3->texture);
			holder3->shotHeight = holder3->texture.getSize().y;
			holder3->shotWidth = holder3->texture.getSize().x;
			holder3->speed = entity->shotSpeed;
			holder3->type = 0;
			holder3->velocity = sf::Vector2f(0, 0.1f);;
			holder3->entityShot.setOrigin(holder3->entityShot.getLocalBounds().width / 2, holder3->entityShot.getLocalBounds().height / 2);
			holder3->entityShot.setPosition(entity->entitySprite.getPosition().x + entity->entityWidth * 0.75f, entity->entitySprite.getPosition().y + entity->entityHeight / 8.0f);
			shotsOnScreen.push_back(holder3); //Push back to the Vector
			shotPool.pop_back(); //Removes from the Shot Pool

			entityShots* holder4 = shotPool.back();
			holder4->texture.loadFromFile("data/EnemyMissile1.png");
			holder4->entityShot.setTexture(holder4->texture);
			holder4->shotHeight = holder4->texture.getSize().y;
			holder4->shotWidth = holder4->texture.getSize().x;
			holder4->speed = entity->shotSpeed;
			holder4->type = 0;
			holder4->velocity = sf::Vector2f(0, 0.1f);;
			holder4->entityShot.setOrigin(holder4->entityShot.getLocalBounds().width / 2, holder4->entityShot.getLocalBounds().height / 2);
			holder4->entityShot.setPosition(entity->entitySprite.getPosition().x - entity->entityWidth * 0.75f, entity->entitySprite.getPosition().y + entity->entityHeight / 8.0f);
			shotsOnScreen.push_back(holder4); //Push back to the Vector
			shotPool.pop_back(); //Removes from the Shot Pool

			entity->timer.restart();
		}
	}
}

void enemyBehavior2(Entity* entity) {
    srand(rand()^time(0));

    //when recently spawned and outside of window, prime for coming in
    if(!entity->isSprited) {
        entity->texture.loadFromFile("data/Enemy3Still.png");
        sf::Vector2u textureSize = entity->texture.getSize();
        entity->entitySprite.setTexture(entity->texture);
        entity->entityHeight = textureSize.y;
        entity->entityWidth = textureSize.x;
        entity->entitySprite.setOrigin(textureSize.x/2.0f, textureSize.y/2.0f);
        entity->isSprited = true;

        entity->shotSpeed = 6.0f;

        enemySpawner(entity);

        entity->entitySprite.setPosition(entity->spawnPosition.x, 0 - entity->entityHeight);
        entity->isComingIn = true;
    }

    //getting to position
    if(entity->isComingIn) {
        entity->velocity = sf::Vector2f(0.0, 0.1f);
        movement(entity);
    }

    //once in place, do its stuff
    if( entity->entitySprite.getPosition().x <= entity->spawnPosition.x + 3.0 &&
        entity->entitySprite.getPosition().x >= entity->spawnPosition.x - 3.0 &&
        entity->entitySprite.getPosition().y <= entity->spawnPosition.y + 3.0 &&
        entity->entitySprite.getPosition().y >= entity->spawnPosition.y - 3.0)
    {
        if(entity->isComingIn) {
            if(rand()%2 == 0) entity->velocity = sf::Vector2f(0.1f, 0.0f);
            else if(rand()%2 == 1) entity->velocity = sf::Vector2f(-0.1f, 0.0f);
            else entity->velocity = sf::Vector2f(0.1f, 0.0f);
        }
        entity->isComingIn = false;
    }

    if(!entity->isComingIn) {
        entity->velocity = sf::Vector2f(entity->velocity.x, 0.0f);
        if(rand()%11 == 0) entity->velocity = sf::Vector2f(-entity->velocity.x, entity->velocity.y);
        float holder = entity->speed;
        entity->speed = 2.0*entity->speed;
        movement(entity);
        entity->speed = holder;

        if (shotPool.size() > 0 && entity->timer.getElapsedTime().asSeconds() > entity->cooldown) {
            entityShots *holder5 = shotPool.back();
			holder5->texture.loadFromFile("data/EnemyMissile1.png");
			holder5->entityShot.setTexture(holder5->texture);
			holder5->shotHeight = holder5->texture.getSize().y;
			holder5->shotWidth = holder5->texture.getSize().x;
			holder5->speed = entity->shotSpeed;
			holder5->velocity = sf::Vector2f(0, 0.1f);
			holder5->type = 0;
			holder5->entityShot.setOrigin(holder5->entityShot.getLocalBounds().width / 2, holder5->entityShot.getLocalBounds().height / 2);
			holder5->entityShot.setPosition(entity->entitySprite.getPosition().x, entity->entitySprite.getPosition().y + entity->entityHeight*0.5f);
			shotsOnScreen.push_back(holder5); //Push back to the Vector
			shotPool.pop_back(); //Removes from the Shot Pool

            entity->timer.restart();
        }
    }
}

//enemy behavior where it follows the player's x coordinate and moves to it along with a limited random y coordinate. Once there, it shoots a large volley and then stops and moves toward the player again.
void enemyBehavior3(Entity* entity, Entity* player) {
    srand(rand()^time(0));

    //when recently spawned and outside of window, prime for coming in
    if(!entity->isSprited) {
        entity->texture.loadFromFile("data/Enemy5StillNEW.png");
        sf::Vector2u textureSize = entity->texture.getSize();
        entity->entitySprite.setTexture(entity->texture);
        entity->entityHeight = textureSize.y;
        entity->entityWidth = textureSize.x;
        entity->entitySprite.setOrigin(textureSize.x/2.0f, textureSize.y/2.0f);
        entity->isSprited = true;

        entity->shotSpeed = 6.0f;

        enemySpawner(entity);

        entity->spawnPosition.x = player->entitySprite.getPosition().x;
        entity->isComingIn = true;
    }

    //getting to position
    if(entity->isComingIn) {
        entity->spawnPosition.x = player->entitySprite.getPosition().x;
        sf::Vector2f direction(entity->spawnPosition.x - entity->entitySprite.getPosition().x, entity->spawnPosition.y - entity->entitySprite.getPosition().y);
        entity->velocity = direction;

        float holder = entity->speed;
        entity->speed = 2*entity->speed;

        movement(entity);
        entity->speed = holder;
        entity->timer2.restart();
    }

    //once in place, do its stuff
    if( entity->entitySprite.getPosition().x <= entity->spawnPosition.x + 2.0 &&
        entity->entitySprite.getPosition().x >= entity->spawnPosition.x - 2.0 &&
        entity->entitySprite.getPosition().y <= entity->spawnPosition.y + 2.0 &&
        entity->entitySprite.getPosition().y >= entity->spawnPosition.y - 2.0)
    {
        entity->isComingIn = false;
    }

        //float holder = entity->cooldown;
        //entity->cooldown = 0.5*entity->cooldown;

    if(!entity->isComingIn) {
        if (shotPool.size() > 0 && entity->timer.getElapsedTime().asSeconds() > entity->cooldown) {
            entityShots* holder1 = shotPool.back();
            holder1->texture.loadFromFile("data/EnemyShotDI.png");
            holder1->entityShot.setTexture(holder1->texture);
            holder1->shotHeight = holder1->texture.getSize().y;
            holder1->shotWidth = holder1->texture.getSize().x;
            holder1->entityShot.setOrigin(holder1->shotWidth/2.0f, holder1->shotHeight/2.0f);
            holder1->speed = entity->shotSpeed;
            holder1->velocity = sf::Vector2f(0, 0.1f);
			holder1->type = 0;
            holder1->entityShot.setPosition(entity->entitySprite.getPosition().x, entity->entitySprite.getPosition().y + entity->entityHeight/2.0f);
            shotsOnScreen.push_back(holder1); //Push back to the Vector
            shotPool.pop_back(); //Removes from the Shot Pool

            entity->timer.restart();
        }

        if(entity->timer2.getElapsedTime().asSeconds() > 3) {
            //entity->spawnPosition.x = player->entitySprite.getPosition().x;
            entity->spawnPosition.y = fmod(rand(), (0.5*screen_h - entity->entityHeight)) + entity->entityWidth/2.0f;
            entity->isComingIn = true;
        }

        //entity->cooldown = holder;
    }
}

void enemyBehavior4(Entity* entity, Entity* player) {
    srand(rand()^time(0));

    //when recently spawned and outside of window, prime for coming in
    if(!entity->isSprited) {
        entity->texture.loadFromFile("data/Enemy4Still.png");
        sf::Vector2u textureSize = entity->texture.getSize();
        entity->entitySprite.setTexture(entity->texture);
        entity->entityHeight = textureSize.y;
        entity->entityWidth = textureSize.x;
        entity->entitySprite.setOrigin(textureSize.x/2.0f, textureSize.y/2.0f);
        entity->isSprited = true;

        entity->shotSpeed = 6.0f;

        enemySpawner(entity);

        entity->spawnPosition.y = fmod(rand(), 0.25*screen_h + entity->entityWidth);
        entity->entitySprite.setPosition(entity->spawnPosition.x, 0 - entity->entityHeight);
        entity->isComingIn = true;
    }

    //getting to position
    if(entity->isComingIn) {
        entity->velocity = sf::Vector2f(entity->velocity.x, 0.1f);
        float holder = entity->speed;
        entity->speed = 0.5*entity->speed;

        movement(entity);
        entity->speed = holder;
    }

    //once in place, do its stuff
    if( entity->entitySprite.getPosition().x <= entity->spawnPosition.x + 2.0 &&
        entity->entitySprite.getPosition().x >= entity->spawnPosition.x - 2.0 &&
        entity->entitySprite.getPosition().y <= entity->spawnPosition.y + 2.0 &&
        entity->entitySprite.getPosition().y >= entity->spawnPosition.y - 2.0)
    {
        if(entity->isComingIn) {
            if(rand()%2 == 0) entity->velocity = sf::Vector2f(0.1f, entity->velocity.y);
            else if(rand()%2 == 1) entity->velocity = sf::Vector2f(-0.1f, entity->velocity.y);
            else entity->velocity = sf::Vector2f(0.1f, entity->velocity.y);
        }
        entity->isComingIn = false;
    }

    if(!entity->isComingIn) {
        entity->velocity = sf::Vector2f(entity->velocity.x, 0.0f);
        if(rand()%101 == 0) entity->velocity = sf::Vector2f(-entity->velocity.x, 0.0f);
        float holder = entity->speed;
        entity->speed = 0.5*entity->speed;
        movement(entity);
        entity->speed = holder;

        float cdHolder = entity->cooldown;
        float speedHolder = entity->shotSpeed;
        if(player->entitySprite.getPosition().x < entity->entitySprite.getPosition().x + entity->entityWidth && player->entitySprite.getPosition().x > entity->entitySprite.getPosition().x - entity->entityWidth) {
            entity->cooldown = 0.05f;
            entity->shotSpeed = 2.0*entity->shotSpeed;
        }
        else entity->cooldown = 3.0*entity->cooldown;

        if (shotPool.size() > 0 && entity->timer.getElapsedTime().asSeconds() > entity->cooldown) {
            entityShots* holder1 = shotPool.back();
            holder1->texture.loadFromFile("data/EnemyShot1.png");
            holder1->entityShot.setTexture(holder1->texture);
            holder1->shotHeight = holder1->texture.getSize().y;
            holder1->shotWidth = holder1->texture.getSize().x;
            holder1->entityShot.setOrigin(holder1->shotWidth/2.0f, holder1->shotHeight/2.0f);
            holder1->speed = entity->shotSpeed;
            holder1->velocity = sf::Vector2f(0, 0.1f);
			holder1->type = 0;
            holder1->entityShot.setPosition(entity->entitySprite.getPosition().x, entity->entitySprite.getPosition().y + entity->entityHeight/2.0f);
            shotsOnScreen.push_back(holder1); //Push back to the Vector
            shotPool.pop_back(); //Removes from the Shot Pool
            entity->timer.restart();
        }

        entity->cooldown = cdHolder;
        entity->shotSpeed = speedHolder;
    }
}

void enemyBehavior5(Entity* entity, Entity* player) {
    srand(rand()^time(0));

    //when recently spawned and outside of window, prime for coming in
    if(!entity->isSprited) {
        entity->texture.loadFromFile("data/Enemy1StillNEW.png");
        sf::Vector2u textureSize = entity->texture.getSize();
        entity->entitySprite.setTexture(entity->texture);
        entity->entityHeight = textureSize.y;
        entity->entityWidth = textureSize.x;
        entity->entitySprite.setOrigin(textureSize.x/2.0f, textureSize.y/2.0f);
        entity->isSprited = true;

        entity->shotSpeed = 0.0f;
        entity->timer2.restart();

        enemySpawner(entity);

        entity->entitySprite.setPosition((rand()%2)*window.getSize().x, entity->spawnPosition.y);
        entity->isComingIn = true;
    }

    //getting to position
    if(entity->isComingIn) {
        if(entity->entitySprite.getPosition().x < entity->spawnPosition.x) entity->velocity = sf::Vector2f(0.1f, entity->velocity.y);
        else if(entity->entitySprite.getPosition().x > entity->spawnPosition.x) entity->velocity = sf::Vector2f(-0.1f, entity->velocity.y);

        movement(entity);
        entity->timer2.restart();
    }

    //once in place, do its stuff
    if( entity->entitySprite.getPosition().x <= entity->spawnPosition.x + 2.0 &&
        entity->entitySprite.getPosition().x >= entity->spawnPosition.x - 2.0 &&
        entity->entitySprite.getPosition().y <= entity->spawnPosition.y + 2.0 &&
        entity->entitySprite.getPosition().y >= entity->spawnPosition.y - 2.0)
    {
        entity->isComingIn = false;
    }

    if(!entity->isComingIn) {
        entity->velocity = sf::Vector2f(0.0f, 0.0f);

        if (shotPool.size() > 0 && entity->timer.getElapsedTime().asSeconds() > entity->cooldown) {
            entityShots* holder1 = shotPool.back();
            holder1->texture.loadFromFile("data/EnemyShot2.png");
            holder1->entityShot.setTexture(holder1->texture);
            holder1->shotHeight = holder1->texture.getSize().y;
            holder1->shotWidth = holder1->texture.getSize().x;
            holder1->entityShot.setOrigin(holder1->shotWidth/2.0f, holder1->shotHeight/2.0f);
			holder1->type = 0;
            holder1->speed = entity->shotSpeed;
            holder1->entityShot.setPosition(entity->entitySprite.getPosition().x, entity->entitySprite.getPosition().y + entity->entityHeight/2.0f);

            holder1->velocity = sf::Vector2f(player->entitySprite.getPosition().x - holder1->entityShot.getPosition().x, player->entitySprite.getPosition().y - holder1->entityShot.getPosition().y);
            normalize(holder1->velocity);
            holder1->velocity = sf::Vector2f(holder1->velocity.x*0.3, holder1->velocity.y*0.3);

            shotsOnScreen.push_back(holder1); //Push back to the Vector
            shotPool.pop_back(); //Removes from the Shot Pool

            entity->timer.restart();
        }

        if(entity->timer2.getElapsedTime().asSeconds() > 3) {
            entity->spawnPosition.x = fmod(rand(), 801-entity->entityWidth) + entity->entityWidth/2.0f;
            entity->isComingIn = true;
            entity->timer2.restart();
        }
    }
}

void enemyBehavior6(Entity* entity, Entity* player) {
    srand(rand()^time(0));

    //when recently spawned and outside of window, prime for coming in
    if(!entity->isSprited) {
        entity->texture.loadFromFile("data/Enemy6StillNEW.png");
        sf::Vector2u textureSize = entity->texture.getSize();
        entity->entitySprite.setTexture(entity->texture);
        entity->entityHeight = textureSize.y;
        entity->entityWidth = textureSize.x;
        entity->entitySprite.setOrigin(textureSize.x/2.0f, textureSize.y/2.0f);
        entity->isSprited = true;

        entity->shotSpeed = 6.0f;
        entity->timer2.restart();

        enemySpawner(entity);

        entity->entitySprite.setPosition((rand()%2)*window.getSize().x, entity->spawnPosition.y);
        entity->isComingIn = true;
    }

    //getting to position
    if(entity->isComingIn) {
        if(entity->entitySprite.getPosition().x < entity->spawnPosition.x) entity->velocity = sf::Vector2f(0.1f, entity->velocity.y);
        else if(entity->entitySprite.getPosition().x > entity->spawnPosition.x) entity->velocity = sf::Vector2f(-0.1f, entity->velocity.y);

        movement(entity);
        entity->timer2.restart();
    }

    //once in place, do its stuff
    if( entity->entitySprite.getPosition().x <= entity->spawnPosition.x + 2.0 &&
        entity->entitySprite.getPosition().x >= entity->spawnPosition.x - 2.0 &&
        entity->entitySprite.getPosition().y <= entity->spawnPosition.y + 2.0 &&
        entity->entitySprite.getPosition().y >= entity->spawnPosition.y - 2.0)
    {
        entity->isComingIn = false;
    }

    if(!entity->isComingIn) {
        entity->velocity = sf::Vector2f(0.0f, 0.0f);

        float cdholder = entity->cooldown;
        entity->cooldown = 0.0f;

        if (shotPool.size() > 0 && entity->timer.getElapsedTime().asSeconds() > entity->cooldown) {
            entityShots* holder1 = shotPool.back();
            holder1->texture.loadFromFile("data/EnemyShot3.png");
            holder1->entityShot.setTexture(holder1->texture);
            holder1->shotHeight = holder1->texture.getSize().y;
            holder1->shotWidth = holder1->texture.getSize().x;
            holder1->entityShot.setOrigin(holder1->shotWidth/2.0f, holder1->shotHeight/2.0f);
            holder1->type = 0;
            holder1->speed = entity->shotSpeed;
            holder1->velocity = sf::Vector2f(0.0f, 0.1f);
            holder1->entityShot.setPosition(entity->entitySprite.getPosition().x, entity->entitySprite.getPosition().y + entity->entityHeight/2.0f);

            shotsOnScreen.push_back(holder1); //Push back to the Vector
            shotPool.pop_back(); //Removes from the Shot Pool

            entity->timer.restart();
        }

        entity->cooldown = cdholder;

        if(entity->timer2.getElapsedTime().asSeconds() > 0.5) {
            entity->spawnPosition.x = fmod(rand(), 801-entity->entityWidth) + entity->entityWidth/2.0f;
            entity->isComingIn = true;
            entity->timer2.restart();
        }
    }
}

int main()
{

	window.setFramerateLimit(FPS);
	window.setKeyRepeatEnabled(false);

	//creates uniform grid of cell_colnum by cell_rownum
	for (int i = 0; i < cell_rownum; i++)
	{
		std::vector<std::vector<entityShots*> > holder;
		for (int j = 0; j < cell_colnum; j++)
		{
			std::vector<entityShots*> holder2;
			holder.push_back(holder2);
		}
		uniform_grid.push_back(holder);
		holder.clear();
	}

	sf::Texture playerTexture;
	sf::Texture bg;
	bg.loadFromFile("data/bg2.png");
	sf::Sprite background(bg);
	playerTexture.loadFromFile("data/BrandNewPlayerModelX.png");

	Entity player;
	player.entitySprite.setTexture(playerTexture);
	player.entityHeight = 64;
	player.entityWidth = 64;
	player.entitySprite.setOrigin(player.entityWidth / 2.0f, player.entityHeight / 2.0f);
	player.speed = 200.0f;
	player.spawnPosition.x = screen_w / 2.0f;
	player.spawnPosition.y = screen_h * 0.9f;
	player.entitySprite.setPosition(player.spawnPosition);
	player.cooldown = 0.1f;
	Entity* playerPTR = &player;

	sf::Texture shot;
	shot.loadFromFile("data/playerShotD.png");

	int num_enemies = 0;
	std::cin >> num_enemies;
	//enemies.resize(num_enemies);
    //enemiesReal.resize(num_enemies);

	for (int i = 0; i<num_enemies; i++)
	{
		int num_behavior = 0;
		std::cin >> num_behavior;
		enemies.push_back(new Entity);
		enemies[i]->speed = 200.0f;
		enemies[i]->cooldown = 0.5f;
		behaviorAssigner(enemies[i], num_behavior);
	}

	for (int i = 0; i < shotPoolSize; i++)
	{
	    shotPool.push_back(new entityShots);
		shotPool[i]->entityShot.setPosition(1000, 1000); //place offscreen
	}

	bool isEscaping = false;
	sf::Clock clock;
	while (window.isOpen()) {
		sf::Event event;
		sf::Time timer = clock.restart();
		float dt = clock.getElapsedTime().asSeconds();

		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) isEscaping = true;
			if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::Escape) isEscaping = true;
			}
		}

		window.clear(sf::Color::Black);
		window.draw(background);

		for(int i=0; i<enemies.size(); i++) {
            if(enemies[i]->behavior == 1) enemyBehavior1(enemies[i]);
            else if(enemies[i]->behavior == 2) enemyBehavior2(enemies[i]);
            else if(enemies[i]->behavior == 3) enemyBehavior3(enemies[i], playerPTR);
            else if(enemies[i]->behavior == 4) enemyBehavior4(enemies[i], playerPTR);
            else if(enemies[i]->behavior == 5) enemyBehavior5(enemies[i], playerPTR);
            else if(enemies[i]->behavior == 6) enemyBehavior6(enemies[i], playerPTR);
            else enemyBehavior1(enemies[i]);
        }

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) { player.velocity.y = -1.0;} //force / player.Mass; }
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) { player.velocity.y = 1.0;} //force / player.Mass; }
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { player.velocity.x = 1.0;} //force / player.Mass; }
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) { player.velocity.x = -1.0;} //force / player.Mass; }

		movement(playerPTR);

		player.velocity = sf::Vector2f(0.0f, 0.0f);

		//IsShooting State
		IsShooting = sf::Keyboard::isKeyPressed(sf::Keyboard::Space) ? true : false;
		if (IsShooting)
		{
			if (shotPool.size() > 0)
			{
				if (!AlternateShot1 && !AlternateShot2)
				{ // Default Shot Type - Double Bullet
					//if (player.timer.getElapsedTime().asSeconds() > player.cooldown)
					//{
						entityShots* holder = shotPool.back();
						holder->texture.loadFromFile("data/PlayerShot1.png");
						holder->entityShot.setTexture(holder->texture);
						holder->shotHeight = holder->texture.getSize().y;
						holder->shotWidth = holder->texture.getSize().x;
						holder->speed = 6.0f;
						holder->type = 1;
						holder->velocity = sf::Vector2f(0, -0.1f);;
						holder->entityShot.setPosition(player.entitySprite.getPosition().x + 10, player.entitySprite.getPosition().y + 5);
                        shotsOnScreen.push_back(holder); //Push back to the Vector
						shotPool.pop_back(); //Removes from the Shot Pool

						entityShots* holder0 = shotPool.back();
						holder0->texture.loadFromFile("data/PlayerShot1.png");
						holder0->entityShot.setTexture(holder0->texture);
						holder0->shotHeight = holder0->texture.getSize().y;
						holder0->shotWidth = holder0->texture.getSize().x;
						holder0->speed = 6.0f;
						holder0->type = 1;
						holder0->velocity = sf::Vector2f(0, -0.1f);;
						holder0->entityShot.setPosition(player.entitySprite.getPosition().x - 10, player.entitySprite.getPosition().y + 5);
						shotsOnScreen.push_back(holder0); //Push back to the Vector
						shotPool.pop_back(); //Removes from the Shot Pool

						player.timer.restart();
					//}

				}
			}
		}

		for (int i = shotsOnScreen.size() - 1; i > -1; i--)
		{
			shotsOnScreen[i]->update(timer.asMilliseconds());
			sf::Vector2f pos = shotsOnScreen[i]->entityShot.getPosition();
			float hw = shotsOnScreen[i]->entityShot.getLocalBounds().width / 2; float hh = shotsOnScreen[i]->entityShot.getLocalBounds().height / 2;
			if (pos.x + hw < 0 || pos.x - hw > screen_w || pos.y + hh < 0 || pos.y - hh > screen_h) {
				shotsOnScreen[i]->entityShot.setPosition(1000, -1000);
				shotPool.push_back(shotsOnScreen[i]);
				shotsOnScreen.erase(shotsOnScreen.begin() + i);
				delete shotPool[shotPool.size()-1];
				shotPool[shotPool.size()-1] = new entityShots;
				continue;
			}
			if (shotsOnScreen[i]->type == 0) //type 1 is playershot; type 0 is enemyshot
			{
				gridPlacement(shotsOnScreen[i], shotsOnScreen[i]->shotHeight);
			}

		}

	begin: for (int i = shotsOnScreen.size() - 1; i > -1; i--)
	{
		/// Only check if there's a bullet in that current cell
		if (shotsOnScreen[i]->type > 0)
		{
			entityShots *p = shotsOnScreen[i];
			/// Determine where the player would have been if it were in the uniform grid
			sf::Vector2f pos = p->entityShot.getPosition();
			int minr = (pos.y - (shot_h / 2)) / cell_h;
			int maxr = (pos.y + (shot_h / 2)) / cell_h;
			int minc = (pos.x - (shot_w / 2)) / cell_w;
			int maxc = (pos.x + (shot_w / 2)) / cell_w;
			/// Check if out of bounds
			if (maxr > cell_rownum - 1)
			{
				maxr = cell_rownum - 1;
			}
			else
			{
				maxr = maxr;
			}
			if (maxc > cell_colnum - 1)
			{
				maxc = cell_colnum - 1;
			}
			else
			{
				maxc = maxc;
			}
			if (minr < 0)
			{
				minr = 0;
			}
			else
			{
				minr = minr;
			}
			if (minc < 0)
			{
				minc = 0;
			}
			else
			{
				minc = minc;
			}
			/// Check all of the cells the bullet would have been in
			for (int i = minr; i <= maxr; i++)
			{
				for (int j = minc; j <= maxc; j++)
				{
					std::vector<entityShots*> cellsToCheck = uniform_grid[i][j];
					for (int k = 0; k < cellsToCheck.size(); k++)
					{
						entityShots *b = cellsToCheck[k];
						collisionHappen(p, b);
					}
					cellsToCheck.clear();
				}
			}
		}
	}

		   for (int i = shotsOnScreen.size() - 1; i > -1; i--)
		   {
			   sf::Vector2f pos = shotsOnScreen[i]->entityShot.getPosition();
			   //shotsOnScreen[i]->entityShot.move(shotsOnScreen[i]->speed*shotsOnScreen[i]->velocity.x*TIMESTEP, shotsOnScreen[i]->speed*shotsOnScreen[i]->velocity.y*TIMESTEP);
			   if (pos.x > 0 - shotsOnScreen[i]->shotWidth && pos.x < screen_w + shotsOnScreen[i]->shotWidth &&
				   pos.y > 0 - shotsOnScreen[i]->shotHeight && pos.y < screen_h + shotsOnScreen[i]->shotHeight)
			   {
				   window.draw(shotsOnScreen[i]->entityShot);
			   }
			   else
			   {
				   shotPool.push_back(shotsOnScreen[i]); //Shots return to the pool
				   shotsOnScreen.erase(shotsOnScreen.begin() + i); //Shots removed from screen
				   delete shotPool[shotPool.size()-1];
                    shotPool[shotPool.size()-1] = new entityShots;
			   }
		   }

		   for (int i = 0; i<enemies.size(); i++)
		   {
			   window.draw(enemies[i]->entitySprite);
		   }

		   window.draw(player.entitySprite);
		   window.display();

		   clearGrid();

		if(isEscaping) break;
	}

	clearGrid();
	uniform_grid.clear();
	shotPool.clear();
	enemies.clear();
	shotsOnScreen.clear();
	window.close();
	return 0;
}
