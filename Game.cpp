#include "Game.h"

#include <iostream>
#include <fstream>

Game::Game(const std::string& config) // constructor
{
	init(config);
}

void Game::init(const std::string& path)
{
	// TODO: read in config file here
	// use the premade struct variables for PlayerConfig, etc
	std::ifstream fin(path);

	// fin >> _playerConfig.SR >> _playerConfig.CR >> ...

	_window.create(sf::VideoMode(1280, 720), "Polyshooter");
	_window.setFramerateLimit(60);

	spawnPlayer();
}

void Game::run()
{
	while (_running)
	{
		_entityManager.update();

		if (!_player->getStatus())
		{
			spawnPlayer();
		}

		if (!_paused) // what to do when we aren't paused
		{
			sEnemySpawner();
			sMovement();
			sCollision();
			sUserInput();
			sLifespan();
		}

		sRender();
		
		_currentFrame++; // increment current frame
	}
}

void Game::setPaused(bool paused)
{
	_paused = paused;
}

void Game::spawnPlayer()
{
	auto entity = _entityManager.addEntity("player");

	float mx = _window.getSize().x / 2.0f;
	float my = _window.getSize().y / 2.0f;
	
	// spawns at (200, 200) with velocity of (1, 1) at angle 0
	entity->cTransform = std::make_shared<CTransform>(Vec2(mx, my), Vec2(1.0f, 1.0f), 0.0f);

	// shape with radius 32 and 8 sides, dark grey fill and red border with 4 border thickness
	entity->cShape = std::make_shared<CShape>(32.0f, 8, sf::Color(10, 10, 10), sf::Color(255, 0, 0), 4.0f);

	// add input component, so we can move it
	entity->cInput = std::make_shared<CInput>();

	// add wall collision
	entity->cCollision = std::make_shared<CCollision>(32.0f);

	_player = entity; // set this entity to our player
}

void Game::spawnEnemy()
{
	// create enemy similar to player
	auto entity = _entityManager.addEntity("enemy");

	int radius = 22, minSides = 3, maxSides = 10; // min and max sides of the polygon

	float randomSides = minSides + (rand() % (1 + maxSides - minSides)); // choose random polygon [3-10]

	float ex = radius + (rand() % (1 + _window.getSize().x - radius)); // give it a min and max thats equal to the radius of the object
	float ey = radius + (rand() % (1 + _window.getSize().y - radius)); // give it a min and max thats equal to the radius of the object

	float r = rand() % 255, g = rand() % 255, b = rand() % 255;
	float outline_r = rand() % 255, outline_g = rand() % 255, outline_b = rand() % 255;
	float speed = 4 + rand() % (1 + 4);

	entity->cTransform = std::make_shared<CTransform>(Vec2(ex, ey), Vec2(speed, speed), 5.0f);
	entity->cShape = std::make_shared<CShape>(radius, randomSides, sf::Color(r, g, b), sf::Color(outline_r, outline_g, outline_b), 4.0f);
	entity->cCollision = std::make_shared<CCollision>(18.0f);

	// record when the most recent enemy was spawned
	_lastEnemySpawnTime = _currentFrame;
}

void Game::spawnSmallEnemies(std::shared_ptr<Entity> e)
{
	// get origin of polygon
	Vec2 origin = e->cTransform->pos;
	int sides = e->cShape->circle.getPointCount();
	float angle = 360.0f / sides;
	float radius = e->cShape->circle.getRadius();
	float speed = 3.0f;
	// in order to get the trajectory angle aka n.x and n.y, we will need to do math!

	for (int i = 0; i < sides; i++)
	{
		// get current trajectory angle
		float currentAngle = angle * i;

		// get the d.x and d.y and store them
		Vec2 vec = { radius * cosf(currentAngle), radius * sinf(currentAngle) };
		
		// normalize vector
		vec = vec.normalize(radius);
		auto entity = _entityManager.addEntity("enemy");
		entity->cTransform = std::make_shared<CTransform>(Vec2(e->cTransform->pos.x, e->cTransform->pos.y), (vec * speed), 0.0f);
		entity->cShape = std::make_shared<CShape>(e->cShape->circle.getRadius() / 2.0f, sides, sf::Color(255,255,255), sf::Color(255, 255, 255), 4.0f);
		//entity->cCollision = std::make_shared<CCollision>(18.0f);
		entity->cLifespan = std::make_shared<CLifespan>(30.0f);
	}
}

void Game::spawnBullet(std::shared_ptr<Entity> entity, const Vec2& mousePos)
{
	auto bullet = _entityManager.addEntity("bullet");

	// in order to get velocity we must first find the mx,my and px,py
	// calculate the distance, 
	float distance = entity->cTransform->pos.dist(mousePos);
	
	// get the (m.x-p.x, m.y-p.y) = D
	Vec2 newVec = entity->cTransform->pos.length(mousePos);
	
	// normalize the vector to get nx, ny
	Vec2 normalizedVec = newVec.normalize(distance);
	
	// we also need the angle (theta) of the trajectory!
	float angle = normalizedVec.theta();

	// then multiply by speed vector
	float speed = 20.0f;

	// spawn the bullet at the position of the player entity
	bullet->cTransform = std::make_shared<CTransform>(Vec2(entity->cTransform->pos.x, entity->cTransform->pos.y), (Vec2(normalizedVec.x, normalizedVec.y) * speed), 1.0);

	bullet->cShape = std::make_shared<CShape>(10.0f, 8, sf::Color(210, 135, 255), sf::Color(255, 255, 255), 2.0f);
	
	// give the bullet collision
	bullet->cCollision = std::make_shared<CCollision>(11.0f);

	// bullet lasts 60 frames
	bullet->cLifespan = std::make_shared<CLifespan>(60.0f); 
}

void Game::spawnSpecialWeapon(std::shared_ptr<Entity> e)
{
	// get origin of polygon
	Vec2 origin = e->cTransform->pos;
	int sides = e->cShape->circle.getPointCount();
	float angle = 360.0f / sides;
	float radius = e->cShape->circle.getRadius();
	float speed = 5.0f;
	// in order to get the trajectory angle aka n.x and n.y, we will need to do math!

	for (int i = 0; i < sides; i++)
	{
		// get current trajectory angle
		float currentAngle = angle * i;

		// get the d.x and d.y and store them
		Vec2 vec = { radius * cosf(currentAngle), radius * sinf(currentAngle) };

		// normalize vector
		vec = vec.normalize(radius);
		auto entity = _entityManager.addEntity("bullet");
		entity->cTransform = std::make_shared<CTransform>(Vec2(e->cTransform->pos.x, e->cTransform->pos.y), (vec * speed), 0.0f);
		entity->cShape = std::make_shared<CShape>(e->cShape->circle.getRadius() / 3.0f, sides, sf::Color::Red, sf::Color::Red, 2.0f);
		entity->cCollision = std::make_shared<CCollision>(18.0f);
		entity->cLifespan = std::make_shared<CLifespan>(30.0f);
	}
}

void Game::sRender()
{
	_window.clear();

	for (auto e : _entityManager.getEntities())
	{
		e->cShape->circle.setPosition(e->cTransform->pos.x, e->cTransform->pos.y);

		e->cTransform->angle += 1.0f;

		e->cShape->circle.setRotation(e->cTransform->angle);

		_window.draw(e->cShape->circle);
	}

	_window.display();
}

void Game::sMovement()
{

	for (auto& e : _entityManager.getEntities()) // movement of every entitty
	{
		e->cTransform->pos.x += e->cTransform->velocity.x;
		e->cTransform->pos.y += e->cTransform->velocity.y;
	}

	_player->cTransform->velocity = { 0, 0 };

	// player change directions
	if (_player->cInput->up)
	{
		_player->cTransform->velocity.y = -5; // TODO: change this to speed later
	}
	if (_player->cInput->down)
	{
		_player->cTransform->velocity.y = 5; 
	}
	if (_player->cInput->left)
	{
		_player->cTransform->velocity.x = -5;
	}
	if (_player->cInput->right)
	{
		_player->cTransform->velocity.x = 5;
	}

	// player movement
	_player->cTransform->pos.x += _player->cTransform->velocity.x;
	_player->cTransform->pos.y += _player->cTransform->velocity.y;
}

void Game::sCollision()
{
	// TODO: Implement all the proper collisions between
	// entities, be sure to use collision radius, NOT shape radius
	for (auto& e : _entityManager.getEntities())
	{
		if (e->cCollision != NULL)
		{
			if (e->cTransform->pos.x > (_window.getSize().x - e->cCollision->radius) || e->cTransform->pos.x < e->cCollision->radius)
			{
				e->cTransform->velocity.x *= -1.0; // inverse x direction
			}
			if (e->cTransform->pos.y > (_window.getSize().y - e->cCollision->radius) || e->cTransform->pos.y < e->cCollision->radius)
			{
				e->cTransform->velocity.y *= -1.0; // inverse y direction
			}
		}

	}

	// enemy-bullet collision detection
	for (auto& e : _entityManager.getEntities("enemy"))
	{
		for (auto& b : _entityManager.getEntities("bullet"))
		{
			if (e->cCollision != NULL)
			{
				// if bullet collides with enemy, destroy both
				// get length between them
				Vec2 D = b->cTransform->pos.length(e->cTransform->pos);
			
				// see if distance between them is less than their summed radiuses
				if (((D.x * D.x) + (D.y * D.y)) < ((b->cCollision->radius + e->cCollision->radius) * (b->cCollision->radius + e->cCollision->radius)))
				{
					b->destroy();
					e->destroy();
					spawnSmallEnemies(e);
				}
			}
		}
	}

	// when an enemy runs into the player
	for (auto& e : _entityManager.getEntities("enemy"))
	{
		if (e->cCollision != NULL)
		{
			Vec2 D = _player->cTransform->pos.length(e->cTransform->pos);

			// see if distance between them is less than their summed radiuses
			if (((D.x * D.x) + (D.y * D.y)) < ((_player->cCollision->radius + e->cCollision->radius) * (_player->cCollision->radius + e->cCollision->radius)))
			{
				e->destroy();
				_player->destroy();
				spawnSmallEnemies(e);
				spawnSmallEnemies(_player);
			}
		}
	}
}

void Game::sEnemySpawner()
{
	if (_currentFrame - _lastEnemySpawnTime > 70.0f) // spawns every 70 frames
	{
		spawnEnemy();
	}
}

void Game::sLifespan()
{
	for (auto& e : _entityManager.getEntities())
	{
		if (e->cLifespan != NULL)
		{
			e->cLifespan->remaining -= 1; // remove a tick from the entities lifespan
			
			if (e->cLifespan->remaining > 0)
			{
				float transparency = (e->cLifespan->remaining * 255) / e->cLifespan->total;
				e->cShape->circle.setFillColor(sf::Color(255, 255, 255, transparency));
				e->cShape->circle.setOutlineColor(sf::Color(255, 255, 255, transparency));
			}

			if (e->cLifespan->remaining < 0)// if no more lifespan, destroy it
			{
				e->destroy();
			}
		}
	}
}

void Game::sUserInput()
{
	sf::Event event;
	while (_window.pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
		{
			_running = false;
		}

		if (event.type == sf::Event::KeyPressed) // key pressed down
		{
			switch (event.key.code)
			{
			case sf::Keyboard::W:
				std::cout << "W Key was pressed\n";
				_player->cInput->up = true;
				break;
			case sf::Keyboard::A:
				std::cout << "A Key was pressed\n";
				_player->cInput->left = true;
				break;
			case sf::Keyboard::S:
				std::cout << "S Key was pressed\n";
				_player->cInput->down = true;
				break;
			case sf::Keyboard::D:
				std::cout << "D Key was pressed\n";
				_player->cInput->right = true;
				break;
			default: break;
			}
		}

		if (event.type == sf::Event::KeyReleased) // key released
		{
			switch (event.key.code)
			{
			case sf::Keyboard::W:
				std::cout << "W Key was released\n";
				_player->cInput->up = false;
				break;
			case sf::Keyboard::A:
				std::cout << "A Key was released\n";
				_player->cInput->left = false;
				break;
			case sf::Keyboard::S:
				std::cout << "S Key was released\n";
				_player->cInput->down = false;
				break;
			case sf::Keyboard::D:
				std::cout << "D Key was released\n";
				_player->cInput->right = false;
				break;
			default: break;
			}
		}

		if (event.type == sf::Event::MouseButtonPressed) // mouse clicks
		{
			if (event.mouseButton.button == sf::Mouse::Left)
			{
				std::cout << "Left mouse clicked at (" << event.mouseButton.x << ", " << event.mouseButton.y << ")\n";
				spawnBullet(_player, Vec2(event.mouseButton.x, event.mouseButton.y));
			}
			
			if (event.mouseButton.button == sf::Mouse::Right)
			{
				std::cout << "Right mouse clicked at (" << event.mouseButton.x << ", " << event.mouseButton.y << ")\n";
				spawnSpecialWeapon(_player);
			}
		}
	}
}


