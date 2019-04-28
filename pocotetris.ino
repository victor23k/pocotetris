#include <Adafruit_GFX.h> // Core graphics library
#include <Adafruit_TFTLCD.h> // Specific graphics library
#include <MCUFRIEND_kbv.h>   // Specific graphics library

#define ANALOG_BUTTON A5  // Pin analogico para los botones
#define LCD_RESET A4  // Can alternately just connect to Arduino's reset pin
#define LCD_CS A3    // Chip Select goes to Analog 3
#define LCD_CD A2   // Command/Data goes to Analog 2
#define LCD_WR A1  // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

// Valores de colores en 16-bits:
#define BLACK 0x0000
#define NAVY 0x000F
#define DARKGREEN 0x03E0
#define DARKCYAN 0x03EF
#define MAROON 0x7800
#define PURPLE 0x780F
#define OLIVE 0x7BE0
#define LIGHTGREY 0xC618
#define DARKGREY 0x7BEF
#define BLUE 0x001F
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define ORANGE 0xFD20
#define GREENYELLOW 0xAFE5
#define PINK 0xF81F

// Array para colores de los tetrominos (0 será vacío)
uint16_t t_colors[8] = {BLACK, CYAN, BLUE, ORANGE, YELLOW, GREEN, PURPLE, RED};

// Medidas del campo de juego en bloques (incluyendo limites), sin limites mide 10*20
int nFieldWidth = 12;
int nFieldHeight = 23;
// Campo de juego
int *pField = nullptr;

//	7 total pieces, 4x4 space for each piece
int tetromino[7][16] = {
	
	{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1}, //i block
	{0,0,0,0,0,0,0,0,2,0,0,0,2,2,2,0}, //J block
	{0,0,0,0,0,0,0,0,0,0,0,3,0,3,3,3}, //L block
	{0,0,0,0,0,0,0,0,0,4,4,0,0,4,4,0}, //O block
	{0,0,0,0,0,0,0,0,0,0,5,5,0,5,5,0}, //S block
	{0,0,0,0,0,0,0,0,0,6,0,0,6,6,6,0}, //T block
	{0,0,0,0,0,0,0,0,7,7,0,0,0,7,7,0}, //Z block
	
};

// Botones digitales
const int a_button = 13;
const int up_button = 12;
const int b_button = 11;
const int left_button = 10;


int rotate(int px, int py, int r){
	switch (r % 4){
		case 0: return py * 4 + px;		// 0 grados
		case 1: return 12 + py - (px * 4);	// 90 grados
		case 2: return 15 - (py * 4) - px; 	// 180 grados
		case 3: return 3 - py + (px * 4);	// 270 grados
	}
	return 0;
}

bool doesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY)
{
	// All Field cells >0 are occupied
	for (int px = 0; px < 4; px++)
		for (int py = 0; py < 4; py++)
		{
			// Get index into piece
			int pi = rotate(px, py, nRotation);

			// Get index into field
			int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

			// Check that test is in bounds. Note out of bounds does
			// not necessarily mean a fail, as the long vertical piece
			// can have cells that lie outside the boundary, so we'll
			// just ignore them
			if (nPosX + px >= 0 && nPosX + px < nFieldWidth)
			{
				if (nPosY + py >= 0 && nPosY + py < nFieldHeight)
				{
					// In Bounds so do collision check
					if (tetromino[nTetromino][pi] !=0 && pField[fi] != 0)
						return false; // fail on first hit
				}
			}
		}

	return true;
}

MCUFRIEND_kbv tft; //320*480


/*
	Displays the template for the game, score, next and hold pieces
*/
void display_gameboard(void){
	int16_t x = 60, y = 40, w = 200, h = 400;
	// Game rectangle
	tft.drawRect(x-1, y-1, w+2, h+2, DARKGREY);
	tft.drawRect(x-2, y-2, w+8, h+8, DARKGREY);
	// Guías
	for (int ax = x; ax < w+x; ax += 20){
		tft.drawFastVLine(ax, y, h, LIGHTGREY);
	}

	for (int ay = y; ay < h+y; ay += 20){
		tft.drawFastHLine(x, ay, w, LIGHTGREY);
	}

	tft.setTextColor(GREEN);
	tft.setTextSize(1);

	tft.setCursor(2, 2);
	tft.println("SCORE");
	
	tft.setCursor(2, 50);
	tft.println("LEVEL");
	
	tft.setCursor(2, 100);
	tft.println("LINES");
	
	
	// Next piece
	tft.setCursor(280, 150);
	tft.println("NEXT");

	// Hold piece
	tft.setCursor(280, 2);
	tft.println("HOLD");
	
}

/*
	Draws a block
*/
void draw_block(int16_t x, int16_t y, uint16_t color){
	int16_t w = 20, h = 20;
    x = x * 20 + 40;
    y = y * 20;
    if (color != BLACK){
		tft.fillRect(x+1, y+1, w-1, h-1, color);
    }
	else{
		if (y > 20){
			if (y < 440)
				tft.drawRect(x, y, w, h, LIGHTGREY);
			tft.fillRect(x+1, y+1, w-1, h-1, BLACK);
		}
	}
}

/* 
	Elegimos la siguiente pieza de forma aleatoria
*/
int randomizePiece(int currPiece){
	int randomPiece = random(7);
	if (randomPiece == currPiece || randomPiece == 7)
		randomPiece = random(6);
	return randomPiece;
}

void setup(void) {
	// Configuramos los botones digitales como botones de entrada
	pinMode(a_button, INPUT);
	pinMode(up_button, INPUT);
	pinMode(b_button, INPUT);
	pinMode(left_button, INPUT);

	//Reseteamos la pantalla para evitar errores
	tft.reset();

	uint16_t identifier = tft.readID();
	
	//Inicializamos la pantalla con su driver correspondiente
	tft.begin(identifier); 

}

void loop(void) {

	tft.fillScreen(BLACK);

// Bienvenida del juego

	tft.setCursor(40, 200);
	tft.setTextColor(YELLOW);
	tft.setTextSize(4);
	tft.println("POCOTETRIS");
	tft.setTextSize(2);
	tft.setCursor(50, 300);
	tft.setTextColor(RED);
	tft.print("PULSE");
  	tft.setTextColor(GREEN);
	tft.print(" A ");
  	tft.setTextColor(RED);
  	tft.print("PARA JUGAR");

	int a_value = 0;

// Hasta que el usuario no pulse el boton no se inicia el juego
	while (a_value == 0){
		a_value = digitalRead(a_button);
	}

// Usamos el tiempo que tarda el usuario en inciar el juego como seed para generar números aleatorios
	randomSeed(millis());
	
	tft.fillScreen(BLACK);

// Empieza el juego
	display_gameboard();

	pField = new int[nFieldWidth*nFieldHeight]; // Reservo memoria para el campo de juego
	for (int x = 0; x < nFieldWidth; x++){ 
		for (int y = 0; y < nFieldHeight; y++){ // Meto los huecos y los límites del campo de juego
			pField[y*nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;
		}
	}
	
	// Variables que nos ayudan a controlar el juego
	bool gameOver = false;
	bool forceDown = false;
	bool moveMade = false;
	bool rotateHold = true;
	bool holdingPiece = false;
	
	int nCurrentPiece = 0;
	int nextPiece = 0;
	int nCurrentRotation = 0;
	int nCurrentX = nFieldWidth / 2;
	int nCurrentY = 0;
	int nSpeed = 20;
	int nSpeedCount = 0;
	int holdPiece = 9; // Inicializamos a este valor para no sacar nada
    int nPieceCount = 0;
    int totalScore = 0;
    int currentScore = 0;
    int currentColor = 0;
    int linesCounter = 0;
    int currentLines = 0;
    int currentLevel = 1;
    int analogValue = 0;

	unsigned long currentTime = 0;
	long previousTime = 0;
	long interval = 50;

	/* Randomize first pieces
		Como la pieza se inicializa a 0, será menos probable que salga el tetromino I, 
		pero no hay problema ya que este es el más poderoso y es la primera pieza de la partida.
	*/
	nCurrentPiece = randomizePiece(nCurrentPiece); 
	nextPiece = randomizePiece(nCurrentPiece);
	
	while (!gameOver){
		// GAME TIMING
		nSpeedCount+=currentLevel;
		forceDown = (nSpeedCount >= nSpeed);
		moveMade = false;

		// En vez de hacer un delay de 50 que sería un tick del juego,
		// empleamos esos 50ms para leer los botones
		currentTime = millis();
		previousTime = currentTime;

		while (currentTime - previousTime < interval){
			currentTime = millis();

			// INPUT
			analogValue = analogRead(ANALOG_BUTTON);
			
			//mover izquierda
			if(digitalRead(left_button) == 1 && !moveMade){  
		        if (doesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)){
					nCurrentX--;
					moveMade = true;
		        }
	      	}
			//mover derecha
			else if(analogValue > 300 && analogValue < 420 && !moveMade){
				if (doesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)){
					nCurrentX++;
					moveMade = true;
				} 
			}	
			//mover abajo
			else if(analogValue > 190 && analogValue < 270 && !moveMade){	
				if (doesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)){
					nCurrentY++;
					moveMade = true;
				}
			}
			//rotar pieza
			else if (digitalRead(a_button) == 1 && !moveMade){
				if (doesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY + 1)){
					nCurrentRotation++;
					if(nCurrentY == 0) nCurrentY++; // Para evitar el fallo de la rotación que se sube
					moveMade = true;
				}
			}
			//guardar pieza
			if (digitalRead(b_button) == 1 && !moveMade && !holdingPiece){
				if (holdPiece == 9){
					holdPiece = nCurrentPiece;
					nCurrentPiece = nextPiece;
					nextPiece = randomizePiece(nCurrentPiece);
				} else{
					int auxPiece = nCurrentPiece;
					nCurrentPiece = holdPiece;
					holdPiece = auxPiece;
				}	
				holdingPiece = true;
			}

			//mover pieza abajo del todo
			else if(digitalRead(up_button) == 1 && !moveMade){

		        while(doesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)){
					nCurrentY++;
					moveMade = true;
		        }
	              
			}
				
			// Ejecutamos el movimiento en cuanto el jugador pulsa el boton
			if (moveMade) break;

		}
		

		if (forceDown){

			// Actualizar dificultad cada 50 piezas encajadas
			nSpeedCount = 0;
			nPieceCount++;
			if (nPieceCount % 50 == 0)
				if (nSpeed >= 10) nSpeed--;

			// Si la pieza cabe, sigue bajando
			if (doesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)){
				nCurrentY++;
			}else{

				// Bloquear la pieza en el suelo
				for (int px = 0; px < 4; px++)
					for (int py = 0; py < 4; py++)
						if (tetromino[nCurrentPiece][rotate(px, py, nCurrentRotation)] != 0)
							pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;

      		  	// Comprobar lineas
                for (int py = 0; py < 4; py++){
                    if(nCurrentY + py < nFieldHeight - 1){
                      bool line = true;
                      for (int px = 1; px < nFieldWidth - 1; px++)
                        line &= (pField[(nCurrentY + py) * nFieldWidth + px]) != 0;
          
                      if (line){
                        currentLines++;
                      }           
                    }
				}
				
				// Pintar las lineas completas de gris
				linesCounter += currentLines;
				int lastLine = nFieldHeight - 1 - currentLines;
				for (int py = nFieldHeight - 1; py > lastLine; py--){
					for (int px = 1; px < nFieldWidth - 1; px++){
						draw_block(px, py, LIGHTGREY);
					}
				}
				delay(100);
				for (int py = nFieldHeight - 1; py > lastLine; py--){
					for (int px = 1; px < nFieldWidth - 1; px++){
						draw_block(px, py, BLACK);
					}
				}
				// Bajar tablero
				for (int pn = 0; pn < currentLines; pn++){
					for (int py = nFieldHeight - 2; py > 0; py--){
						for (int px = 1; px < nFieldWidth - 1; px++){
							pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
						}
					}
				}

				currentScore += 25;

	            switch (currentLines){
	            	case 1: currentScore += 100;
	            	case 2: currentScore += 300;
	            	case 3: currentScore += 500;
	            	case 4: currentScore += 800; 
	            }
	            
	            currentScore *= currentLevel;
	            totalScore += currentScore;
	            linesCounter += currentLines / 2;

	            currentScore = 0;
	            currentLines = 0;

     
				// nueva pieza 
				nCurrentX = nFieldWidth / 2;
				nCurrentY = 0;
				nCurrentRotation = 0;
				nCurrentPiece = nextPiece;
				nextPiece = randomizePiece(nCurrentPiece);
				
				// Si la pieza nueva no cabe al principio, se termina el juego
				gameOver = !doesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);

				// Como hemos colocado una pieza, ya se puede volver a cambiar
				holdingPiece = false;
			}
		}
		

		// RENDER OUTPUT

		/* La salida solo cambiará si:
					- hemos hecho un movimiento
					- la pieza baja por tiempo
			así evitamos un refresco constante
		*/


		if (moveMade || forceDown){
			// Dibujar el campo
		    for (int x = 0; x < nFieldWidth; x++){
		      for (int y = 0; y < nFieldHeight; y++){
		      	currentColor = pField[y*nFieldWidth + x];
		      	if(currentColor != 9)
		        	draw_block(x, y, t_colors[currentColor]);
		      }
			}

		    // Dibujar pieza actual
		    for (int px = 0; px < 4; px++){
		      for (int py = 0; py < 4; py++){
		        currentColor = tetromino[nCurrentPiece][rotate(px, py, nCurrentRotation)];
		        if (currentColor != 0)
		          draw_block(nCurrentX + px, nCurrentY + py, t_colors[currentColor]);
		      }
		    }

		    // Dibujar pieza siguiente

		    for (int px = 0; px < 4; px++){
		    	for (int py = 1; py < 4; py++){
		    		currentColor = tetromino[nextPiece][py * 4 + px];
		    		tft.fillRect(280 + px * 10, 155 + py * 10, 10, 10, t_colors[currentColor]);
		    	}
		    }

		    // Dibujar pieza on hold

		    if (holdPiece != 9) { // Si el valor de holdPiece no ha cambiado, no hace falta pintar nada
		    	for (int px = 0; px < 4; px++){
			    	for (int py = 1; py < 4; py++){
			    		currentColor = tetromino[holdPiece][py * 4 + px];
			    		tft.fillRect(280 + px * 10, 2 + py * 10, 10, 10, t_colors[currentColor]);
			    	}
			    }
		    }
		}


		// Mostrar puntuacion, nivel y lineas
		tft.setCursor(5, 10);
		tft.fillRect(5, 10, 20, 10, BLACK);
		tft.println(totalScore);

		tft.setCursor(5, 58);
		tft.fillRect(5, 58, 20, 10, BLACK);
		tft.println(currentLevel);

		tft.setCursor(5, 108);
		tft.fillRect(5, 108, 20, 10, BLACK);
		tft.println(linesCounter);

	}

	// Al perder la partida, esta se acaba y mostramos un cartel de game over durante dos segundos
	tft.setCursor(100, 200);
	tft.setTextColor(GREEN);
	tft.setTextSize(2);
	tft.fillRect(80, 0, 320, 60, BLACK);
	tft.println("GAME OVER");

	/*
		MEJORAS POSIBLES PARA IMPLEMENTAR:
			- sombra de la pieza actual en el suelo
	*/

	delay(2000);
	
	// Después vuelve a empezar el juego
}
