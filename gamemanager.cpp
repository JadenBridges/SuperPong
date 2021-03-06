#include "gamemanager.h"
#include "mainwindow.h"
#include <unistd.h>
#include <iostream>

using namespace std;

void *thread_producer_fn(void *);
void *thread_consumer_fn(void *);

typedef struct thread_arguments
{
    Message<Ball> *ballMessage_p;
    Message<Powerup> *powerupMessage_p;
    MainWindow *w_p;
    bool gameActive;
    int winner;
    GameManager *game_manager;
    int powerup_state;
} thread_arguments_t;

GameManager::GameManager()
{
    // power-up spawn timer goes off after 10 seconds
    powerup_spawn_timer.setTimerSize(7);
    powerup_spawn_timer.setTokenTime(1000000);
    powerup_spawn_timer.resetTimer();   // start this timer immediately, runs for entirety of game

    // power-up active timer goes off after 10 seconds
    powerup_active_timer.setTimerSize(10);
    powerup_active_timer.setTokenTime(1000000);
}

int GameManager::run(MainWindow& w)
{
    thread_arguments_t arguments;

    // threads
    pthread_t thread_consumer, thread_producer;
    Message<Ball> ballMessage;
    Message<Powerup> powerupMessage;

    arguments.ballMessage_p = &ballMessage;
    arguments.powerupMessage_p = &powerupMessage;
    arguments.w_p = &w;
    arguments.gameActive = true;
    arguments.winner = 0;
    arguments.game_manager = this;
    arguments.powerup_state = 0;

    if(pthread_create(&thread_producer, NULL, thread_producer_fn, &arguments) != 0)
    {
        perror("Thread producer creation failed.");
        exit(1);
    }
    if(pthread_create(&thread_consumer, NULL, thread_consumer_fn, &arguments) != 0)
    {
        perror("Thread consumer creation failed.");
        exit(1);
    }

    pthread_join(thread_producer, NULL);
    pthread_join(thread_consumer, NULL);

    return arguments.winner;
}

void *thread_producer_fn(void *args)
{
    thread_arguments_t *arguments = (thread_arguments_t *)args;

    Message<Ball> *ballMessage = arguments->ballMessage_p;
    Message<Powerup> *powerupMessage = arguments->powerupMessage_p;
    MainWindow *window = arguments->w_p;
    bool *gameActive = &arguments->gameActive;
    int *winner = &arguments->winner;
    GameManager *game_manager = arguments->game_manager;
    int *powerup_state = &arguments->powerup_state;

    Ball ball;
    Powerup powerup;
    int lastWallCollided_ball = 0;
    int lastWallCollided_powerup = 0;
    int lastPaddleCollided = 0;
    int lastGoalCollided = 0;
    int activePowerup = 0;
    int powerupCollidedPaddle = -1;
    bool powerup1Changed[4] = {0,0,0,0};
    bool powerup1XYPositive[2] = {0, 0};
    bool powerup1VelocityChanged = 0;
    //int lastSpeedSetting = window->checkSelectedGameSpeed();

    window->moveBall(ball.getLocation().first, ball.getLocation().second);

    int increase_speed = 0;
    int when_increase_speed = 10000000;

    while(*gameActive) {

        /**********************************************/
        /***************** POWERUP ********************/

        // if there is not a currently active powerup
        if(activePowerup == 0)
        {
            // check if a powerup needs spawned or despawned
            if(game_manager->powerup_spawn_timer.processTimer())
            {
                *powerup_state = (*powerup_state+1)%6;
                int state = *powerup_state;

                switch(state)
                {
                    case 0:
                        window->despawnPowerup();
                        break;
                    case 1:
                        window->spawnPowerup(1);
                        lastGoalCollided = 0;
                        powerup.setLocationCenter();
                        window->centerPowerup();
                        break;
                    case 2:
                        window->despawnPowerup();
                        break;
                    case 3:
                        window->spawnPowerup(2);
                        lastGoalCollided = 0;
                        powerup.setLocationCenter();
                        window->centerPowerup();
                        break;
                    case 4:
                        window->despawnPowerup();
                        break;
                    case 5:
                        window->spawnPowerup(3);
                        lastGoalCollided = 0;
                        powerup.setLocationCenter();
                        window->centerPowerup();
                }
                powerup.setXVelocity(-powerup.getXVelocity());
            } // if(game_manager->powerup_spawn_timer.processTimer())


            // if it is in a state where a powerup is visible
            if(*powerup_state == 1 || *powerup_state == 3 || *powerup_state == 5)
            {
                // reset to center if it has just been spawned

                powerup.move();
                powerupMessage->putMessage(powerup);

                // check if powerup had a wall collision
                int currentPowerupWallCollision = window->checkPowerupWallCollision();
                if (lastWallCollided_powerup != currentPowerupWallCollision && currentPowerupWallCollision != 0)
                {
                    powerup.collideWall();
                    lastWallCollided_powerup = currentPowerupWallCollision;
                }
                // check if powerup had a goal collision
                int currentPowerupGoalCollision = window->checkPowerupGoalCollision();
                if (lastGoalCollided != currentPowerupGoalCollision && currentPowerupGoalCollision != 0)
                {
                    powerup.collideGoal();
                    lastGoalCollided = currentPowerupGoalCollision;
                }
                // check if powerup had a paddle collision
                powerupCollidedPaddle = window->checkPowerupPaddleCollision();
                if (powerupCollidedPaddle != 0)
                {
                    // set the active powerup
                    activePowerup = *powerup_state == 1 ? 1 : *powerup_state == 3 ? 2 : 3;
                    if (activePowerup == 1)
                    {
                        if(ball.getXVelocity() > 0 && ball.getXVelocity() > SPEED_OFFSET)
                        {
                            powerup1VelocityChanged = true;
                        }
                        else if (ball.getXVelocity() < 0 && ball.getXVelocity() < -SPEED_OFFSET)
                        {
                            powerup1VelocityChanged = true;
                        }
                        if(ball.getYVelocity() > 0 && ball.getYVelocity() < SPEED_OFFSET)
                        {
                            powerup1VelocityChanged = false;
                        }
                        else if(ball.getYVelocity() < 0 && ball.getYVelocity() > -SPEED_OFFSET)
                        {
                            powerup1VelocityChanged = false;
                        }

                        if(powerup1VelocityChanged) {
                            if(ball.getXVelocity() > 0 && ball.getXVelocity() > SPEED_OFFSET)
                            {
                                ball.setXVelocity(ball.getXVelocity() - SPEED_OFFSET);
                                powerup1Changed[0] = true;
                                powerup1XYPositive[0] = true;
                            }
                            else if(ball.getXVelocity() < -SPEED_OFFSET)
                            {
                                ball.setXVelocity(ball.getXVelocity() + SPEED_OFFSET);
                                powerup1Changed[1] = true;
                            }
                            if(ball.getYVelocity() > 0 && ball.getYVelocity() > SPEED_OFFSET)
                            {
                                ball.setYVelocity(ball.getYVelocity() - SPEED_OFFSET);
                                powerup1Changed[2] = true;
                                powerup1XYPositive[1] = true;
                            }
                            else if(ball.getYVelocity() < -SPEED_OFFSET)
                            {
                                ball.setYVelocity(ball.getYVelocity() + SPEED_OFFSET);
                                powerup1Changed[3] = true;
                            }
                        }
                    }
                    // activate the corresponding powerup
                    window->activatePowerup(activePowerup, powerupCollidedPaddle);

                    // start the active powerup timer
                    game_manager->powerup_active_timer.resetTimer();
                }
            } // if(*powerup_state == 1 || *powerup_state == 3 || *powerup_state == 5)
        } // if(activePowerup == 0)

        // if there is currently an active powerup and it needs to be deactivated
        else if(game_manager->powerup_active_timer.processTimer())
        {
            // deactivate the powerup
            if (activePowerup == 1)
            {
                if(powerup1VelocityChanged)
                {
                    if(ball.getXVelocity() > 0 && powerup1XYPositive[0]) {
                        if(powerup1Changed[0])
                        {
                            ball.setXVelocity(ball.getXVelocity() + SPEED_OFFSET);
                            powerup1Changed[0] = false;
                        }
                        else if(powerup1Changed[1])
                        {
                            ball.setXVelocity(ball.getXVelocity() - SPEED_OFFSET);
                            powerup1Changed[1] = false;
                        }
                        powerup1XYPositive[0] = false;
                    }
                    else {
                        if(powerup1Changed[0])
                        {
                            ball.setXVelocity(ball.getXVelocity() - SPEED_OFFSET);
                            powerup1Changed[0] = false;
                        }
                        else if(powerup1Changed[1])
                        {
                            ball.setXVelocity(ball.getXVelocity() + SPEED_OFFSET);
                            powerup1Changed[1] = false;
                        }
                    }
                    if(ball.getYVelocity() > 0 && powerup1XYPositive[1])
                    {
                        if(powerup1Changed[2])
                        {
                            ball.setYVelocity(ball.getYVelocity() + SPEED_OFFSET);
                            powerup1Changed[2] = false;
                        }
                        else if(powerup1Changed[3])
                        {
                            ball.setYVelocity(ball.getYVelocity() - SPEED_OFFSET);
                            powerup1Changed[3] = false;
                        }
                        powerup1XYPositive[1] = false;
                    }
                    else
                    {
                        if(powerup1Changed[2])
                        {
                            ball.setYVelocity(ball.getYVelocity() - SPEED_OFFSET);
                            powerup1Changed[2] = false;
                        }
                        else if(powerup1Changed[3])
                        {
                            ball.setYVelocity(ball.getYVelocity() + SPEED_OFFSET);
                            powerup1Changed[3] = false;
                        }
                    }
                }
            }

            window->deactivatePowerup(activePowerup);

            // set the active powerup to none
            activePowerup = 0;
            *powerup_state = (*powerup_state+1)%6;
        }

        /***************** POWERUP ********************/
        /**********************************************/

        /**********************************************/
        /****************** BALL **********************/

        // update ball variables
        ball.move();

        // increase speed gradually
        if(increase_speed == when_increase_speed-1) {
            ball.increaseSpeed();
        }
        increase_speed = (increase_speed+1)%when_increase_speed;

        ballMessage->putMessage(ball);

        // check if ball had a wall collision
        int currentBallWallCollision = window->checkBallWallCollision();
        if (lastWallCollided_ball != currentBallWallCollision && currentBallWallCollision != 0)
        {
            ball.collideWall();
            lastWallCollided_ball = currentBallWallCollision;
        }
        // check if ball had a paddle collision
        int currentBallPaddleCollision = window->checkBallPaddleCollision();
        if (lastPaddleCollided != currentBallPaddleCollision && currentBallPaddleCollision != 0)
        {
            ball.collidePaddle();
            lastPaddleCollided = currentBallPaddleCollision;
        }
        // check if ball had a goal collision
        int ballCollidedGoal = window->checkBallGoalCollision();
        if (ballCollidedGoal != 0)
        {
            *winner = (ballCollidedGoal == 1) ? 2 : 1;
            *gameActive = false;
            for(int i = 1; i < 4; ++i) {
                window->deactivatePowerup(i);
            }
            window->despawnPowerup();
            window->gameOver(*winner);
        }

        /****************** BALL **********************/
        /**********************************************/
    }
    usleep(1000);
    pthread_exit(NULL);
}

void *thread_consumer_fn(void *args)
{
    thread_arguments_t *arguments = (thread_arguments_t *)args;

    Message<Ball> *ballMessage = arguments->ballMessage_p;
    Message<Powerup> *powerupMessage = arguments->powerupMessage_p;
    MainWindow *window = arguments->w_p;
    bool *gameActive = &arguments->gameActive;
    //int *powerup_state = &arguments->powerup_state;

    Ball ball;
    Powerup powerup;

    while(*gameActive) {

        if(ballMessage->getMessage(ball))
        {
            window->moveBall(ball.getLocation().first, ball.getLocation().second);
        }
        if(powerupMessage->getMessage(powerup))
        {
            window->movePowerup(powerup.getLocation().first, powerup.getLocation().second);
        }
        usleep(30000);
    }
    pthread_exit(NULL);
}
