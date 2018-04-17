///////////////////////////////////////
/// trkSettings array
///////////////////////////////////////

void initTrkSettings() {

  // initialise trkSettings array
  for (int i = 0; i < 24; i++) {
    for (int j = 0; j < 10; j++) {
      if (j == 0) { // volume
        trkSettings[i][j] = 215;
      } else if (j == 1) {  // panning
        trkSettings[i][j] = 127;
      } else {
        trkSettings[i][j] = 0;
      }
    }
  }

}


//-------------------------------------------------------
void writeTrkSettings(int col, int row, int value) {
  trkSettings[col][row] = value;
}


//-------------------------------------------------------
int readTrkSettings(int col, int row) {
  int value = trkSettings[col][row];
  return value;
}

