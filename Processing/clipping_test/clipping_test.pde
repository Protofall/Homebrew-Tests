float[] vert_x;
float[] vert_y;
float offset_x;
float offset_y;
color[] colour;
float mid_x, mid_y;
float angle;
String msg;

int ARR_SIZE = 64;

boolean renderMode;  // false is no cropping, true means cropping. Press "R" to toggle this
int moveSpeed;  //Toggle up with 'O' and down with 'P'
boolean debug;

void setup(){
  //size(640,480);
  size(960,720);
  textSize(32);
  
  int high = width / 3;
  int wide = high / 2;
  mid_x = width/2.0;
  mid_y = height/2.0;
  
  offset_x = 0;
  offset_y = 0;
  
  // top left, top right, bottom right, bottom left
  vert_x = new float[4];
  vert_y = new float[4];
  vert_x[0] = mid_x - (wide/2);
  vert_y[0] = mid_y - (high/2);
  vert_x[1] = mid_x + (wide/2);
  vert_y[1] = mid_y - (high/2);
  vert_x[2] = mid_x + (wide/2);
  vert_y[2] = mid_y + (high/2);
  vert_x[3] = mid_x - (wide/2);
  vert_y[3] = mid_y + (high/2);
  
  angle = 0;
  noStroke();
  frameRate(60);
  
  renderMode = true;
  moveSpeed = 1;
  debug = false;
  
  msg = "Hii";  // Default so it doesn't error-out
  
  println("Controls: WASD moves red box. Q rotates anti clockwise, E does clockwise.");
  println("O Increases movement speed, P decreases it. R will enable/disable cropping algorithm.");
  println("Arrow keys will change size of the red box. V for debug");
}

// I *think* this will get the true coordinate for the given vertex...I think
float get_x_1(float cos_theta, float sin_theta, float vert_x, float vert_y){
  return (cos_theta * (vert_x - mid_x)) - (sin_theta * (vert_y - mid_y)) + mid_x;
}

float get_y_1(float cos_theta, float sin_theta, float vert_x, float vert_y){
  return (sin_theta * (vert_x - mid_x)) + (cos_theta * (vert_y - mid_y)) + mid_y;
}

float [] get_range(float [] vals){
  float min = vals[0];
  float max = vals[0];
  for(int i = 1; i < 4; i++){  // Skip the first element since we've already checked it kinda
    if(vals[i] < min){min = vals[i];}
    if(vals[i] > max){max = vals[i];}  // I believe this would work fine with an else if, because in *this* case its only used on the AABB poly and the first vert is the min and the 3rd vert is the max. But it must be an "if" for general cases
  }
  float[] ret = new float[2];
  ret[0] = min;
  ret[1] = max;
  return ret;
}

// The dot product. Produces a scalar (Single value).
// The dot product of X and Y is the length of the projection of A onto B multiplied by the length of B (or the other way around).
// https://physics.stackexchange.com/questions/14082/what-is-the-physical-significance-of-dot-cross-product-of-vectors-why-is-divi
float dot(float x1, float y1, float x2, float y2){
  return (x1*x2) + (y1*y2);
}

// We *could* make this not a function since its only used once
float magnitude(float x, float y){
  return sqrt(pow(x,2) + pow(y,2));
}

float[] unit_vector(float x, float y){
  float magnitude = magnitude(x,y);
  float[] ret = new float[2];
  ret[0] = x / magnitude;
  ret[1] = y / magnitude;
  return ret;
}

// True if overlap, False if they don't
boolean seperating_axis_theorem(float[] x1, float[] y1, float[] x2, float[] y2, float[] normal){
  float[] range = new float[4];  //minAlong1, maxAlong1, minAlong2, maxAlong2
  range[0] = Float.MAX_VALUE;
  range[1] = -Float.MAX_VALUE;
  range[2] = Float.MAX_VALUE;
  range[3] = -Float.MAX_VALUE;
  
  // Dot is projecting the points onto the seperating axis and then we get the max and min
  float dotVal;
  for(int i = 0; i < 4; i++){
    dotVal = dot(x1[i], y1[i], normal[0], normal[1]);
    if(dotVal < range[0]){range[0] = dotVal;}
    if(dotVal > range[1]){range[1] = dotVal;}
    
    dotVal = dot(x2[i], y2[i], normal[0], normal[1]);
    if(dotVal < range[2]){range[2] = dotVal;}
    if(dotVal > range[3]){range[3] = dotVal;}
  }
  
  // max1 < min2 || min1 > max2
  if(range[1] < range[2] || range[0] > range[3]){
    return false;
  }
  
  return true;
}

// True if two shapes overlap, False otherwise
// AABB = Axis Aligned Boundry Box (Camera). OBB = Oriented Boundry Box (Sprite)
// Two shapes only intersect if they have overlap when "projected" onto every normal axis of both shapes.
// OBB-OBB uses 8 normal/seperating axises, one for every side. However AABB-OBB only uses 4 since 4 pairs of axises are always parallel
// For them to intersect, the shapes "crushed" vertexes have to overlap on every axis
  // So if theres even one axis where the crushed vertexes don't intersect, then the whole thing doesn't intersect.
Boolean AABB_OBB_Overlap(float[] OBB_X, float[] OBB_Y, float[] AABB_X, float[] AABB_Y){
  // Checking the camera's normals, simplified since we know its axis aligned so no unit vector calculations needed
  float[] range = get_range(OBB_X);  // Returns min and max of array
  if(range[1] < AABB_X[0] || range[0] > AABB_X[1]){
    return false;
  }
  range = get_range(OBB_Y);
  if(range[1] < AABB_Y[0] || range[0] > AABB_Y[3]){
    return false;
  }
  
  // Checking the sprite's normals
    // This can be more efficient. Just look at what was done here https://gist.github.com/nyorain/dc5af42c6e83f7ac6d831a2cfd5fbece
  float[] normal1 = unit_vector(OBB_X[1] - OBB_X[0], OBB_Y[1] - OBB_Y[0]);  // Left side's normal
  float[] normal2 = unit_vector(OBB_X[3] - OBB_X[0], OBB_Y[3] - OBB_Y[0]);  // Top side's normal
  
  //// Quicker, but they aren't unit vectors
  //// Surprisingly they seem to be slightly more accurate than the previous ones somehow
  //normal1[0] = -(OBB_X[1] - OBB_X[0]);
  //normal1[1] = OBB_Y[1] - OBB_Y[0];
  
  //normal2[0] = -(OBB_X[2] - OBB_X[1]);
  //normal2[1] = OBB_Y[2] - OBB_Y[1];
  
  //// Blue = normal1 (Left side)
  //stroke(#0000FF);
  //line((width/2) - (normal1[0] * 100), (height/2) - (normal1[1] * 100), (width/2) + (normal1[0] * 100), (height/2) + (normal1[1] * 100));
  
  //// Green = normal2 (Top side)
  //stroke(#00FF00);
  //line((width/2) - (normal2[0] * 100), (height/2) - (normal2[1] * 100), (width/2) + (normal2[0] * 100), (height/2) + (normal2[1] * 100));
  
  //if(abs(normal1[0]) != abs(normal2[1]) || abs(normal1[1]) != abs(normal2[0])){
  //  print("THEY DIFFER\n");
  //  print("norm1:" + normal1[0] + ", " + normal1[1] + "\n");
  //  print("norm2:" + normal2[0] + ", " + normal2[1] + "\n");
  //}
  
  if(!seperating_axis_theorem(OBB_X, OBB_Y, AABB_X, AABB_Y, normal1)){
    return false;
  }
  
  if(!seperating_axis_theorem(OBB_X, OBB_Y, AABB_X, AABB_Y, normal2)){
    return false;
  }

  // They overlap
  return true;
}

void renderCamera(float[] x, float[] y){
  fill(0, 0);
  stroke(0);
  line(x[0], y[0], x[1], y[1]);
  line(x[1], y[1], x[2], y[2]);
  line(x[2], y[2], x[3], y[3]);
  line(x[3], y[3], x[0], y[0]);
}

// If we are going here, we're assuming the lines DO intersect somewhere
  // If given x value, we're trying to find y and vica versa
float crayon_graphic_line_plane_intersect(float x0, float y0, float x1, float y1, float curr_val, boolean vertical_plane){
  float m, c;
  if(vertical_plane){  // Have x, finding y
    // To avoid divide by zero error
    if(x0 == x1){  // In our use case, this should never trigger since we know the line couldn't be paralle
      return y0;
    }
    
    // Turn our line segment into "y = mx + c" then plug in x (curr_val) and we have y
    m = (y1 - y0) / (x1 - x0);
    c = y0 - (m * x0);
    
    // We want to get the y value and we have the x value
      // This version doesn't work, but if it did it would be faster than the other method
    //float ratio = abs(x0 - curr_val) / abs(x0 - x1);  // Ratio of how far the point is between x0 and x1
    //return y0 + (abs(y0 - y1) * ratio);   // The y coord
  }
  else{  // Have y, finding x
    // To avoid divide by zero error
    if(y0 == y1){  // Never used for us
      return x0;
    }
    
    // Rather than doing "y = mx + c", I instead do "x = my + c" and this still gets us the right result.
    m = (x1 - x0) / (y1 - y0);
    c = x0 - (m * y0);
  }
  
  return (m * curr_val) + c;
}

// This function might modify holder_vert_x, but it doesn't really matter anyways
int sutherland_hodgman_alg(float[] crop_vert_x, float[] crop_vert_y, float[] camera_x,
    float[] camera_y, int crop_len){
  
  int array_mid = ARR_SIZE / 2;

  // LBRT order
  float camera_min_x = camera_x[0];
  float camera_min_y = camera_y[0];
  float camera_max_x = camera_x[2];
  float camera_max_y = camera_y[2];
  
  int new_len;
  //boolean indexMode = false;  // false = Data is from indexes 0 - 7, true = indexes 8 - 15
  
  // Some vars
  boolean in1, in2;  // True if variable is in
  int i, prev_vert_index;
  float curr_val;
  
  // Left side first
    // I'm going in order v4->v1, v1->v2, v2->v3, v3->v4 so I don't have to modulo check every loop
  new_len = 0;
  curr_val = camera_min_x;
  prev_vert_index = crop_len - 1;
  in1 = (crop_vert_x[prev_vert_index] >= curr_val);  // The last vert
  for(i = 0; i < crop_len; i++){
    in2 = (crop_vert_x[i] >= curr_val);
    if(in1 && in2){  // both in, Save vert i
      crop_vert_x[array_mid + new_len] = crop_vert_x[i];
      crop_vert_y[array_mid + new_len] = crop_vert_y[i];
      new_len++;
    }
    else if(!in1 && in2){  // Make v1', then save v1' then save v2
      crop_vert_x[array_mid + new_len] = curr_val;
      crop_vert_y[array_mid + new_len] = crayon_graphic_line_plane_intersect(crop_vert_x[prev_vert_index], crop_vert_y[prev_vert_index], crop_vert_x[i], crop_vert_y[i], curr_val, true);
      
      crop_vert_x[array_mid + new_len + 1] = crop_vert_x[i];
      crop_vert_y[array_mid + new_len + 1] = crop_vert_y[i];
      new_len += 2;
    }
    else if(in1 && !in2){  // Make v1', then save v1'
      crop_vert_x[array_mid + new_len] = curr_val;
      crop_vert_y[array_mid + new_len] = crayon_graphic_line_plane_intersect(crop_vert_x[prev_vert_index], crop_vert_y[prev_vert_index], crop_vert_x[i], crop_vert_y[i], curr_val, true);
      new_len++;
    }
    // If both are out, we discard both
    
    // Update the prev vert
    in1 = in2;
    prev_vert_index = i;
  }
  if(new_len == 0){  // If we disabled OOB check and poly is entirely OOB, this can happen
    return new_len;
  }
  crop_len = new_len;
  if(debug && crop_len >= 8){
    println("After Left crop, we have " + crop_len + " verts");
  }
  
  
  // -----------------------------
  
  // Stuff
  // First: 400.0, 520.0 (Bottom left)
  // Out-In 400.0, 200.0 (Top left)
  // In-In 560.0, 200.0 (Top Right)
  // In-Out 560.0, 520.0 (Botom Right)
  // Out-Out 400.0, 520.0 (Bottom Left)
  

  // Bottom side
  new_len = 0;
  curr_val = camera_max_y;
  prev_vert_index = (crop_len - 1) + array_mid;
  in1 = (crop_vert_y[prev_vert_index] <= curr_val);  // The last vert
  for(i = array_mid; i < crop_len + array_mid; i++){
    in2 = (crop_vert_y[i] <= curr_val);
    if(in1 && in2){  // both in, Save vert i
      crop_vert_x[new_len] = crop_vert_x[i];
      crop_vert_y[new_len] = crop_vert_y[i];
      new_len++;
    }
    else if(!in1 && in2){  // Make v1', then save v1' then save v2
      crop_vert_x[new_len] = crayon_graphic_line_plane_intersect(crop_vert_x[prev_vert_index], crop_vert_y[prev_vert_index], crop_vert_x[i], crop_vert_y[i], curr_val, false);
      crop_vert_y[new_len] = curr_val;  // This can be the wrong y boarder.
      
      crop_vert_x[new_len + 1] = crop_vert_x[i];
      crop_vert_y[new_len + 1] = crop_vert_y[i];
      new_len += 2;
    }
    else if(in1 && !in2){  // Make v1', then save v1'
      crop_vert_x[new_len] = crayon_graphic_line_plane_intersect(crop_vert_x[prev_vert_index], crop_vert_y[prev_vert_index], crop_vert_x[i], crop_vert_y[i], curr_val, false);
      crop_vert_y[new_len] = curr_val;
      new_len++;
    }
    // If both are out, we discard both
    
    // Update the prev vert
    in1 = in2;
    prev_vert_index = i;
  }
  if(new_len == 0){  // If we disabled OOB check and poly is entirely OOB, this can happen
    return new_len;
  }
  crop_len = new_len;
  if(debug && crop_len >= 8){
    println("After Bottom crop, we have " + crop_len + " verts");
  }
  
  
  // -----------------------------
  
  
  // Right side
  new_len = 0;
  curr_val = camera_max_x;
  prev_vert_index = crop_len - 1;
  in1 = (crop_vert_x[prev_vert_index] <= curr_val);  // The last vert
  for(i = 0; i < crop_len; i++){
    in2 = (crop_vert_x[i] <= curr_val);
    if(in1 && in2){  // both in, Save vert i
      crop_vert_x[array_mid + new_len] = crop_vert_x[i];
      crop_vert_y[array_mid + new_len] = crop_vert_y[i];
      new_len++;
    }
    else if(!in1 && in2){  // Make v1', then save v1' then save v2
      crop_vert_x[array_mid + new_len] = curr_val;
      crop_vert_y[array_mid + new_len] = crayon_graphic_line_plane_intersect(crop_vert_x[prev_vert_index], crop_vert_y[prev_vert_index], crop_vert_x[i], crop_vert_y[i], curr_val, true);
      
      crop_vert_x[array_mid + new_len + 1] = crop_vert_x[i];
      crop_vert_y[array_mid + new_len + 1] = crop_vert_y[i];
      new_len += 2;
    }
    else if(in1 && !in2){  // Make v1', then save v1'
      crop_vert_x[array_mid + new_len] = curr_val;
      crop_vert_y[array_mid + new_len] = crayon_graphic_line_plane_intersect(crop_vert_x[prev_vert_index], crop_vert_y[prev_vert_index], crop_vert_x[i], crop_vert_y[i], curr_val, true);
      new_len++;
    }
    // If both are out, we discard both
    
    // Update the prev vert
    in1 = in2;
    prev_vert_index = i;
  }
  if(new_len == 0){  // If we disabled OOB check and poly is entirely OOB, this can happen
    return new_len;
  }
  crop_len = new_len;
  if(debug && crop_len >= 8){
    println("After Right crop, we have " + crop_len + " verts");
  }
  
  
  // -----------------------------
  
  
  // Top side
  new_len = 0;
  curr_val = camera_min_y;
  prev_vert_index = (crop_len - 1) + array_mid;
  in1 = (crop_vert_y[prev_vert_index] >= curr_val);  // The last vert
  for(i = array_mid; i < crop_len + array_mid; i++){
    in2 = (crop_vert_y[i] >= curr_val);
    if(in1 && in2){  // both in, Save vert i
      crop_vert_x[new_len] = crop_vert_x[i];
      crop_vert_y[new_len] = crop_vert_y[i];
      new_len++;
    }
    else if(!in1 && in2){  // Make v1', then save v1' then save v2
      crop_vert_x[new_len] = crayon_graphic_line_plane_intersect(crop_vert_x[prev_vert_index], crop_vert_y[prev_vert_index], crop_vert_x[i], crop_vert_y[i], curr_val, false);
      crop_vert_y[new_len] = curr_val;
      
      crop_vert_x[new_len + 1] = crop_vert_x[i];
      crop_vert_y[new_len + 1] = crop_vert_y[i];
      new_len += 2;
    }
    else if(in1 && !in2){  // Make v1', then save v1'
      crop_vert_x[new_len] = crayon_graphic_line_plane_intersect(crop_vert_x[prev_vert_index], crop_vert_y[prev_vert_index], crop_vert_x[i], crop_vert_y[i], curr_val, false);
      crop_vert_y[new_len] = curr_val;
      new_len++;
    }
    // If both are out, we discard both
    
    // Update the prev vert
    in1 = in2;
    prev_vert_index = i;
  }
  if(new_len == 0){  // If we disabled OOB check and poly is entirely OOB, this can happen
    return new_len;
  }
  crop_len = new_len;
  if(debug && crop_len >= 8){
    println("After Top crop, we have " + crop_len + " verts");
  }
  
  return new_len;
}

// overlap might go unused
void renderSprite(float[] x, float[] y, float[] camera_x, float[] camera_y){
  
  // If no cropping, just render it
  stroke(#FF0000);
  if(!renderMode){
    line(x[0], y[0], x[1], y[1]);
    line(x[1], y[1], x[2], y[2]);
    line(x[2], y[2], x[3], y[3]);
    line(x[3], y[3], x[0], y[0]);
    return;
  }
  
  // The verts will be copied to here
  int crop_len = 4;
  float[] crop_vert_x = new float[ARR_SIZE];  // I think 8 will be the max vertexes, but we need double because we'll be saving and using arrays at the same time.
  float[] crop_vert_y = new float[ARR_SIZE];
  
  for(int i = 0; i < crop_len; i++){
    crop_vert_x[i] = x[i];
    crop_vert_y[i] = y[i];
  }
  
  for(int i = crop_len; i < ARR_SIZE / 2; i++){
    crop_vert_x[i] = 10;
    crop_vert_y[i] = 10;
  }
  
  crop_len = sutherland_hodgman_alg(crop_vert_x, crop_vert_y, camera_x, camera_y, crop_len);
  
  if(debug){
    println("Currently uses " + crop_len + " verts");
  }
  
  ARR_SIZE = 0;
  for(int i = ARR_SIZE / 2; i < crop_len + (ARR_SIZE / 2) - 1; i++){
    line(crop_vert_x[i], crop_vert_y[i], crop_vert_x[i+1], crop_vert_y[i+1]);
  }
  line(crop_vert_x[crop_len + (ARR_SIZE / 2) - 1], crop_vert_y[crop_len + (ARR_SIZE / 2) - 1], crop_vert_x[ARR_SIZE / 2], crop_vert_y[ARR_SIZE / 2]);  // From v10-15 to v8
  
  //println("It is: " + crop_len);
  //for(int i = ARR_SIZE / 2; i < crop_len + (ARR_SIZE / 2); i++){
  //  println("Coords: " + crop_vert_x[i] + ", " + crop_vert_y[i]);
  //}
  ARR_SIZE = 64;
  
  //// Replace this with the crop_vert_x/y elements
  //line(x[0], y[0], x[1], y[1]);
  //line(x[1], y[1], x[2], y[2]);
  //line(x[2], y[2], x[3], y[3]);
  //line(x[3], y[3], x[0], y[0]);
}

void draw(){
  background(192,192,192);
  
  float rotation_under_360 = angle % 360.0;  //If angle is more than 360 degrees, this fixes that
  if(rotation_under_360 < 0){rotation_under_360 += 360.0;}  //fmod has range -359 to +359, this changes it to 0 to +359
  float angleradians = (rotation_under_360 * PI) / 180.0f;
  float cos_theta = cos(angleradians);
  float sin_theta = sin(angleradians);
  
  //Rotation
  float[] x = new float[4];
  float[] y = new float[4];
  for(int j = 0; j < 4; j++){
    float a = get_x_1(cos_theta, sin_theta, vert_x[j], vert_y[j]);
    float b = get_y_1(cos_theta, sin_theta, vert_x[j], vert_y[j]);
    x[j] = a + offset_x;
    y[j] = b + offset_y;
  }
  
  // Seems this camera stuff could be done in setup instead...
  float[] camera_x = new float[4];
  float[] camera_y = new float[4];
  
  // top left, top right, bottom right, bottom left
  camera_x[0] = mid_x - (width / 4);
  camera_y[0] = mid_y - (height / 5);
  camera_x[1] = mid_x + (width / 4);
  camera_y[1] = mid_y - (height / 5);
  camera_x[2] = mid_x + (width / 4);
  camera_y[2] = mid_y + (height / 5);
  camera_x[3] = mid_x - (width / 4);
  camera_y[3] = mid_y + (height / 5);
  
  // check if they overlap here
  boolean overlap = AABB_OBB_Overlap(x, y, camera_x, camera_y);
  
  renderCamera(camera_x, camera_y);
  renderSprite(x, y, camera_x, camera_y);
  
  // Render text messages
  msg = "Angle: " + angle + " ";
  if(!renderMode){
    msg += "No Crop ";
  }
  else{
    msg += "Cropping ";
  }
  
  if(overlap){
    msg += "Overlaps";
  }
  else{
    msg += "OOB ";
  }
  
  // Render the text
  fill(0);
  textAlign(RIGHT, BOTTOM);
  text(msg, width, height);
  
  textAlign(LEFT, BOTTOM);
  text("Speed: " + moveSpeed + ", Debug: " + (debug ? 1 : 0), 0, height);
}

void keyPressed() {
  if(key == 'R' || key == 'r'){
    renderMode = !renderMode;
  }
  
  if(key == 'O' || key == 'o'){
    if(moveSpeed == 0){
      moveSpeed = 1;
    }
    else{
      moveSpeed = moveSpeed * 2;
    }
  }
  if(key == 'P' || key == 'p'){
    moveSpeed = moveSpeed / 2;
  }
  
  if(key == 'V' || key == 'v'){
    debug = !debug;
  }
  
  if(key == 'E' || key == 'e'){
    angle++;
  }
  if(key == 'Q' || key == 'q'){
    angle--;
  }
  if(key == CODED){
    if(keyCode == UP){
      vert_y[2]++;
      vert_y[3]++;
    }
    if(keyCode == DOWN){
      vert_y[2]--;
      vert_y[3]--;
    }
    if(keyCode == LEFT){
      vert_x[1]--;
      vert_x[3]--;
    }
    if(keyCode == RIGHT){
      vert_x[1]++;
      vert_x[3]++;
    }
  }
  if(key == 'W' || key == 'w'){
    offset_y -= moveSpeed;
  }
  if(key == 'S' || key == 's'){
    offset_y += moveSpeed;
  }
  if(key == 'A' || key == 'a'){
    offset_x -= moveSpeed;
  }
  if(key == 'D' || key == 'd'){
    offset_x += moveSpeed;
  }
}
