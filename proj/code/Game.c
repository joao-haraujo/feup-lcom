#include "Game.h"
#include "Utilities.h"
#include "Mouse.h"

#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/com.h>
#include <stdio.h>
#include <stdlib.h>

Button* createButton(int xi, int yi, int xf, int yf, char* text) {
	Button* button = (Button*) malloc(sizeof(Button));

	button->topLeft = createPosition(xi, yi);
	button->bottomRight = createPosition(xf, yf);

	if (strcmp(text, "") == 0)
		button->text = NULL;
	else
		button->text = loadBitmap(getImagePath(text));

	button->onHover = 0;
	button->onClick = 0;

	return button;
}

void drawButton(Button* button, int highlight) {
	unsigned long colour = COLOUR_BLACK;

	if (getMouse()->draw == 1) {
		if (button->onHover == 1)
			colour = COLOUR_ORANGE;
	} else if (highlight == 1)
		colour = COLOUR_ORANGE;

	drawRectangle(button->topLeft->x, button->topLeft->y,
			button->bottomRight->x, button->bottomRight->y, colour);

	int x = (int) round((button->topLeft->x + button->bottomRight->x) / 2.0);
	int y = button->topLeft->y + 5;
	drawBitmap(button->text, x, y, ALIGN_CENTER);
}

int mouseInButton(Button* button) {
	Mouse* mouse = getMouse();

	if ((button->topLeft->x < mouse->position->x
			&& mouse->position->x < button->bottomRight->x)
			&& (button->topLeft->y < mouse->position->y
					&& mouse->position->y < button->bottomRight->y))
		return 1; //Mouse inside button
	else
		return 0; //Mouse outside button
}

void updateButton(Button* button) {
	if (getMouse()->draw)
		if (mouseInButton(button) == 1) {
			button->onHover = 1;

			if (getMouse()->leftButtonReleased == 1) {
				button->onClick = 1;
			} else
				button->onClick = 0;
		} else {
			button->onHover = 0;
			button->onClick = 0;
		}
	else {
		button->onHover = 0;
		button->onClick = 0;
	}
}

void deleteButton(Button* button) {
	deletePosition(button->topLeft);
	deletePosition(button->bottomRight);
	deleteBitmap(button->text);
	free(button);
}

MainMenu* createMainMenu() {
	MainMenu* mainMenu = (MainMenu*) malloc(sizeof(MainMenu));

	mainMenu->background = loadBitmap((char*) getImagePath("mainMenu"));
	mainMenu->mouseSelection = 0;
	mainMenu->option = -1;
	mainMenu->done = 0;

	Button* arcadeMode = createButton(700, 300, 990, 370, "arcadeMode");
	Button* stageSelect = createButton(700, 380, 990, 450, "stageSelect");
	Button* highScores = createButton(700, 460, 990, 530, "highScores");
	Button* options = createButton(700, 540, 990, 610, "options");
	Button* exit = createButton(700, 620, 990, 690, "exitGame");

	mainMenu->buttons[0] = arcadeMode;
	mainMenu->buttons[1] = stageSelect;
	mainMenu->buttons[2] = highScores;
	mainMenu->buttons[3] = options;
	mainMenu->buttons[4] = exit;

	return mainMenu;
}

void updateMainMenu(MainMenu* mainMenu) {
	if (getMouse()->draw)
		mainMenu->option = -1;

	int i;
	for (i = 0; i < 5; i++) {
		updateButton(mainMenu->buttons[i]);
		if (mainMenu->buttons[i]->onClick) {
			mainMenu->mouseSelection = i + 1;
			resetMouseButton();
		}
	}
}

void drawMainMenu(MainMenu* mainMenu) {
	drawBitmap(mainMenu->background, 0, 0, ALIGN_LEFT);
	int i;
	if (getMouse()->draw) {
		for (i = 0; i < 5; i++) {
			drawButton(mainMenu->buttons[i], 0);
		}
	} else {
		for (i = 0; i < 5; i++) {
			if (mainMenu->option == i)
				drawButton(mainMenu->buttons[i], 1);
			else
				drawButton(mainMenu->buttons[i], 0);
		}
	}
}

void deleteMainMenu(MainMenu* mainMenu) {
	deleteBitmap(mainMenu->background);

	int i;
	for (i = 0; i < 5; i++) {
		deleteButton(mainMenu->buttons[i]);
	}
	free(mainMenu);
}

ArcadeMode* createArcadeMode() {
	ArcadeMode* arcadeMode = (ArcadeMode*) malloc(sizeof(ArcadeMode));

	arcadeMode->score = 0;
	arcadeMode->currentStage = 1;
	arcadeMode->foundWally = 0;

	arcadeMode->pause = 0;
	arcadeMode->pauseContinue = createButton(515, 500, 805, 570,
			"pauseContinue");
	arcadeMode->pauseQuit = createButton(515, 580, 805, 650, "pauseQuit");

	arcadeMode->option = -1;
	arcadeMode->done = 0;

	arcadeMode->findWally = loadBitmap(getImagePath("findWally"));
	arcadeMode->wally = loadBitmap(getImagePath("wally"));
	arcadeMode->checkmark = loadBitmap(getImagePath("checkmark"));

	arcadeMode->timer = createTimer();

	FILE *file = fopen(PATH_STAGE "coordArcade.txt", "r");
	if (file == NULL) {
		LOG("ERROR", "Couldn't open coordinates file.");
	}

	int i = 0;
	char str[100];
	while (fgets(str, 100, file)) {
		int stageNumber = atoi(str);
		fgets(str, 100, file);
		char* sub = strtok(str, " ");
		int x = atoi(sub);
		sub = strtok(NULL, " ");
		int y = atoi(sub);
		Position* position = createPosition(x, y + 213);

		arcadeMode->stages[i] = createStageArcadeMode(stageNumber, position);
		i++;
	}
	fclose(file);

	return arcadeMode;
}

void updateArcadeMode(ArcadeMode* arcadeMode) {
	if (getMouse()->draw)
		arcadeMode->option = -1;

	if (arcadeMode->foundWally) {
		int waiting = arcadeMode->timer->counter - arcadeMode->foundWally;
		if (waiting >= 1 * TIMER_DEFAULT_FREQ) {
			if (arcadeMode->currentStage == 10) {
				arcadeMode->done = 1;
				return;
			}
			arcadeMode->currentStage++;
			arcadeMode->foundWally = 0;
			resetTimer(arcadeMode->timer);
			return;
		}
		return;
	}

	if (arcadeMode->pause) {
		drawRectangle(505, 490, 815, 660, COLOUR_GREY);
		updateButton(arcadeMode->pauseContinue);
		updateButton(arcadeMode->pauseQuit);

		if (arcadeMode->pauseContinue->onClick) {
			arcadeMode->pause = 0;
			resumeTimer(arcadeMode->timer);
		} else if (arcadeMode->pauseQuit->onClick) {
			arcadeMode->done = 1;
			return;
		}
		return;
	}

	Stage* stage = arcadeMode->stages[arcadeMode->currentStage - 1];

	updateStage(stage);

	if (stage->pause->onClick) {
		stopTimer(arcadeMode->timer);
		arcadeMode->pause = 1;
	}

	if (stage->foundWally) {
		drawCircleBorder(stage->wally->x, stage->wally->y, 25, COLOUR_ORANGE);
		arcadeMode->foundWally = arcadeMode->timer->counter;

		int timeLeft = (60 - arcadeMode->timer->counter / TIMER_DEFAULT_FREQ);
		arcadeMode->score += 10 * timeLeft;

		LOG_VAR("Time Left", timeLeft);
		LOG_VAR("Score", arcadeMode->score);
	}

	if (arcadeMode->timer->counter >= 60 * TIMER_DEFAULT_FREQ) {
		arcadeMode->done = 1;
	}
}

void drawArcadeMode(ArcadeMode* arcadeMode) {
	if (arcadeMode->foundWally) {
		drawBitmap(arcadeMode->checkmark, 350, 25, ALIGN_LEFT);
		return;
	}

	if (arcadeMode->pause) {
		if (getMouse()->draw) {
			drawButton(arcadeMode->pauseContinue, 0);
			drawButton(arcadeMode->pauseQuit, 0);
		} else {
			switch (arcadeMode->option) {
			case -1:
				drawButton(arcadeMode->pauseContinue, 0);
				drawButton(arcadeMode->pauseQuit, 0);
				break;
			case 0:
				drawButton(arcadeMode->pauseContinue, 1);
				drawButton(arcadeMode->pauseQuit, 0);
				break;
			case 1:
				drawButton(arcadeMode->pauseContinue, 0);
				drawButton(arcadeMode->pauseQuit, 1);
				break;
			default:
				break;
			}
		}
		return;
	}

	fillDisplay(COLOUR_WHITE);
	drawTimeLeft(arcadeMode->timer, 60);

	drawBitmap(arcadeMode->findWally, 0, 30, ALIGN_LEFT);
	drawBitmap(arcadeMode->wally, 280, 10, ALIGN_LEFT);
	drawStage(arcadeMode->stages[arcadeMode->currentStage - 1]);
}

void deleteArcadeMode(ArcadeMode* arcadeMode) {
	int i;
	for (i = 0; i < 10; i++) {
		deleteStage(arcadeMode->stages[i]);
	}

	deleteButton(arcadeMode->pauseContinue);
	deleteButton(arcadeMode->pauseQuit);
	deleteBitmap(arcadeMode->findWally);
	deleteBitmap(arcadeMode->wally);
	deleteBitmap(arcadeMode->checkmark);
	deleteTimer(arcadeMode->timer);

	free(arcadeMode);
}

StageSelect* createStageSelect() {
	StageSelect* stageSelect = (StageSelect*) malloc(sizeof(StageSelect));

	stageSelect->currentStage = 0;
	stageSelect->foundAll = 0;

	stageSelect->background = loadBitmap(getImagePath("stageSelectMenu"));

	stageSelect->buttons[0] = createButton(150, 240, 560, 500, "stage01");
	stageSelect->buttons[1] = createButton(650, 240, 1060, 500, "stage02");
	stageSelect->buttons[2] = createButton(150, 520, 560, 780, "stage07");
	stageSelect->buttons[3] = createButton(650, 520, 1060, 780, "stage10");
	stageSelect->buttons[4] = createButton(535, 850, 745, 910,
			"mainMenuButton");

	stageSelect->pause = 0;
	stageSelect->pauseContinue = createButton(515, 500, 805, 570,
			"pauseContinue");
	stageSelect->pauseQuit = createButton(515, 580, 805, 650, "pauseQuit");

	stageSelect->mouseSelection = 0;
	stageSelect->option = -1;
	stageSelect->done = 0;

	stageSelect->checkmark = loadBitmap(getImagePath("checkmark"));

	stageSelect->findWally = loadBitmap(getImagePath("findWally"));
	stageSelect->wally = loadBitmap(getImagePath("wally"));
	stageSelect->findWenda = loadBitmap(getImagePath("findWenda"));
	stageSelect->wenda = loadBitmap(getImagePath("wenda"));
	stageSelect->findWhitebeard = loadBitmap(getImagePath("findWhitebeard"));
	stageSelect->whitebeard = loadBitmap(getImagePath("whitebeard"));
	stageSelect->findOdlaw = loadBitmap(getImagePath("findOdlaw"));
	stageSelect->odlaw = loadBitmap(getImagePath("odlaw"));

	stageSelect->timer = createTimer();

	FILE *file = fopen(PATH_STAGE "coordStageSelect.txt", "r");
	if (file == NULL) {
		LOG("ERROR", "Couldn't open coordinates file.");
	}

	int i = 0;
	char str[100];
	while (fgets(str, 100, file)) {
		int stageNumber = atoi(str);
		fgets(str, 100, file);
		char* sub = strtok(str, " ");
		int x = atoi(sub);
		sub = strtok(NULL, " ");
		int y = atoi(sub);
		Position* positionWally = createPosition(x, y + 213);

		fgets(str, 100, file);
		sub = strtok(str, " ");
		x = atoi(sub);
		sub = strtok(NULL, " ");
		y = atoi(sub);
		Position* positionWenda = createPosition(x, y + 213);

		fgets(str, 100, file);
		sub = strtok(str, " ");
		x = atoi(sub);
		sub = strtok(NULL, " ");
		y = atoi(sub);
		Position* positionWhitebeard = createPosition(x, y + 213);

		fgets(str, 100, file);
		sub = strtok(str, " ");
		x = atoi(sub);
		sub = strtok(NULL, " ");
		y = atoi(sub);
		Position* positionOdlaw = createPosition(x, y + 213);

		stageSelect->stages[i] = createStage(stageNumber, positionWally,
				positionWenda, positionWhitebeard, positionOdlaw);
		i++;
	}
	fclose(file);

	return stageSelect;
}
void updateStageSelect(StageSelect* stageSelect) {
	if (getMouse()->draw)
		stageSelect->option = -1;

	if (stageSelect->currentStage) {
		if (stageSelect->foundAll) {
			int waiting = stageSelect->timer->counter - stageSelect->foundAll;
			LOG_VAR("Waiting", waiting);
			if (waiting >= 1 * TIMER_DEFAULT_FREQ) {
				stageSelect->foundAll = 0;
				resetStage(stageSelect->stages[stageSelect->currentStage - 1]);
				stageSelect->currentStage = 0;
				stopTimer(stageSelect->timer);
				return;
			}
			return;
		}

		if (stageSelect->pause) {
			drawRectangle(505, 490, 815, 660, COLOUR_GREY);
			updateButton(stageSelect->pauseContinue);
			updateButton(stageSelect->pauseQuit);

			if (stageSelect->pauseContinue->onClick) {
				stageSelect->pause = 0;
				resumeTimer(stageSelect->timer);
			} else if (stageSelect->pauseQuit->onClick) {
				stageSelect->pause = 0;
				resetStage(stageSelect->stages[stageSelect->currentStage - 1]);
				stageSelect->currentStage = 0;
				stopTimer(stageSelect->timer);
				return;
			}
			return;
		}

		Stage* stage = stageSelect->stages[stageSelect->currentStage - 1];
		updateStage(stage);

		if (stage->pause->onClick) {
			stopTimer(stageSelect->timer);
			stageSelect->pause = 1;
		}

		if (foundAll(stage))
			stageSelect->foundAll = stageSelect->timer->counter;

		if (stageSelect->timer->counter >= 120 * TIMER_DEFAULT_FREQ) {
			resetStage(stageSelect->stages[stageSelect->currentStage - 1]);
			stageSelect->currentStage = 0;
			stopTimer(stageSelect->timer);
		}
	} else {
		if (getMouse()->draw)
			stageSelect->option = -1;

		int i;
		for (i = 0; i < 5; i++) {
			updateButton(stageSelect->buttons[i]);
			if (stageSelect->buttons[i]->onClick) {
				stageSelect->mouseSelection = i + 1;
				resetMouseButton();
			}
		}
	}
}

void drawStageSelect(StageSelect* stageSelect) {
	if (stageSelect->currentStage) {
		if (stageSelect->timer->counter == 0 && !stageSelect->foundAll)
			fillDisplay(COLOUR_WHITE);
		if (stageSelect->stages[stageSelect->currentStage - 1]->foundWally)
			drawBitmap(stageSelect->checkmark, 400, 25, ALIGN_LEFT);
		if (stageSelect->stages[stageSelect->currentStage - 1]->foundWenda)
			drawBitmap(stageSelect->checkmark, 400, 95, ALIGN_LEFT);
		if (stageSelect->stages[stageSelect->currentStage - 1]->foundWhitebeard)
			drawBitmap(stageSelect->checkmark, 830, 25, ALIGN_LEFT);
		if (stageSelect->stages[stageSelect->currentStage - 1]->foundOdlaw)
			drawBitmap(stageSelect->checkmark, 830, 95, ALIGN_LEFT);

		if (stageSelect->pause) {
			if (getMouse()->draw) {
				drawButton(stageSelect->pauseContinue, 0);
				drawButton(stageSelect->pauseQuit, 0);
			} else {
				switch (stageSelect->option) {
				case -1:
					drawButton(stageSelect->pauseContinue, 0);
					drawButton(stageSelect->pauseQuit, 0);
					break;
				case 0:
					drawButton(stageSelect->pauseContinue, 1);
					drawButton(stageSelect->pauseQuit, 0);
					break;
				case 1:
					drawButton(stageSelect->pauseContinue, 0);
					drawButton(stageSelect->pauseQuit, 1);
					break;
				default:
					break;
				}
			}
			return;
		}

		if (!stageSelect->foundAll)
			drawTimeLeft(stageSelect->timer, 120);

		drawBitmap(stageSelect->findWally, 0, 30, ALIGN_LEFT);
		drawBitmap(stageSelect->wally, 380, 10, ALIGN_RIGHT);

		drawBitmap(stageSelect->findWenda, 0, 100, ALIGN_LEFT);
		drawBitmap(stageSelect->wenda, 380, 110, ALIGN_RIGHT);

		drawBitmap(stageSelect->findWhitebeard, 990, 30, ALIGN_LEFT);
		drawBitmap(stageSelect->whitebeard, 900, 10, ALIGN_LEFT);

		drawBitmap(stageSelect->findOdlaw, 990, 100, ALIGN_LEFT);
		drawBitmap(stageSelect->odlaw, 900, 110, ALIGN_LEFT);

		drawStage(stageSelect->stages[stageSelect->currentStage - 1]);
	} else {
		drawBitmap(stageSelect->background, 0, 0, ALIGN_LEFT);
		int i;
		if (getMouse()->draw) {
			for (i = 0; i < 5; i++) {
				drawButton(stageSelect->buttons[i], 0);
			}
		} else {
			for (i = 0; i < 5; i++) {
				if (stageSelect->option == i)
					drawButton(stageSelect->buttons[i], 1);
				else
					drawButton(stageSelect->buttons[i], 0);
			}
		}
	}
}
void deleteStageSelect(StageSelect* stageSelect) {
	int i;
	for (i = 0; i < 4; i++) {
		deleteStage(stageSelect->stages[i]);
	}

	deleteButton(stageSelect->pauseContinue);
	deleteButton(stageSelect->pauseQuit);
	deleteBitmap(stageSelect->checkmark);

	deleteBitmap(stageSelect->findWally);
	deleteBitmap(stageSelect->wally);
	deleteBitmap(stageSelect->findWenda);
	deleteBitmap(stageSelect->wenda);
	deleteBitmap(stageSelect->findWhitebeard);
	deleteBitmap(stageSelect->whitebeard);
	deleteBitmap(stageSelect->findOdlaw);
	deleteBitmap(stageSelect->odlaw);

	deleteTimer(stageSelect->timer);

	free(stageSelect);
}

Stage* createStageArcadeMode(int stageNumber, Position* wally) {
	Stage* stage = (Stage*) malloc(sizeof(Stage));

	stage->arcadeMode = 1;

	stage->foundWally = 0;
	stage->wally = wally;

	stage->pause = createButton(1050, 30, 1240, 100, "pause");
	stage->image = loadBitmap(getStagePath(stageNumber));

	return stage;
}

Stage* createStage(int stageNumber, Position* wally, Position* wenda,
		Position* whitebeard, Position* odlaw) {
	Stage* stage = (Stage*) malloc(sizeof(Stage));

	stage->arcadeMode = 0;

	stage->foundWally = 0;
	stage->wally = wally;
	stage->foundWenda = 0;
	stage->wenda = wenda;
	stage->foundWhitebeard = 0;
	stage->whitebeard = whitebeard;
	stage->foundOdlaw = 0;
	stage->odlaw = odlaw;

	stage->pause = createButton(550, 120, 740, 190, "pause");
	stage->image = loadBitmap(getStagePath(stageNumber));

	return stage;
}

int foundAll(Stage* stage) {
	if (stage->foundWally && stage->foundWenda && stage->foundWhitebeard
			&& stage->foundOdlaw)
		return 1;
	else
		return 0;
}

void updateStage(Stage* stage) {
	updateButton(stage->pause);

	if (stage->pause->onClick) {
		stage->pause->onHover = 0;
		drawButton(stage->pause, 0);
		return;
	}

	Mouse* mouse = getMouse();
	if (mouse->leftButtonReleased == 1) {
		if (calcDistance(mouse->position, stage->wally) < 25)
			stage->foundWally = 1;
		if (!stage->arcadeMode) {
			if (calcDistance(mouse->position, stage->wenda) < 25)
				stage->foundWenda = 1;
			if (calcDistance(mouse->position, stage->whitebeard) < 25)
				stage->foundWhitebeard = 1;
			if (calcDistance(mouse->position, stage->odlaw) < 25)
				stage->foundOdlaw = 1;
		}
	}
}

void drawStage(Stage* stage) {
	drawButton(stage->pause, 0);
	drawBitmap(stage->image, 0, 213, ALIGN_LEFT);

	if (!stage->arcadeMode) {
		if (stage->foundWally) {
			drawCircleBorder(stage->wally->x, stage->wally->y, 25,
			COLOUR_ORANGE);
		}
		if (stage->foundWenda) {
			drawCircleBorder(stage->wenda->x, stage->wenda->y, 25,
			COLOUR_ORANGE);
		}
		if (stage->foundWhitebeard) {
			drawCircleBorder(stage->whitebeard->x, stage->whitebeard->y, 25,
			COLOUR_ORANGE);
		}
		if (stage->foundOdlaw) {
			drawCircleBorder(stage->odlaw->x, stage->odlaw->y, 25,
			COLOUR_ORANGE);
		}
	}
}

void resetStage(Stage* stage) {
	stage->foundWally = 0;
	stage->foundWenda = 0;
	stage->foundWhitebeard = 0;
	stage->foundOdlaw = 0;
}

void deleteStage(Stage* stage) {
	deleteButton(stage->pause);
	deleteBitmap(stage->image);

	deletePosition(stage->wally);
	if (!stage->arcadeMode) {
		deletePosition(stage->wenda);
		deletePosition(stage->whitebeard);
		deletePosition(stage->odlaw);
	}

	free(stage);
}