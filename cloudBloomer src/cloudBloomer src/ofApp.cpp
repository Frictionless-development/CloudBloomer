#include "ofApp.h"

// cloudBloomer based on an idea by Manu Luksch & Jack Wolf. Code by Federico Foderaro, expanded by Mukul Patel.

// Parameters:
float speed = 2;          // Particle explosion speed
float rotationSpeed = 590.0; // the bigger the absolute value of the number, the slower the rotation
float scalingFactor =65; // try 60 for abstract blender object, 70 for cube
float zoffset = 200;     // how far is the mesh, try 0
float yoffset = 2;        // y offset of the mesh, try 20
float xoffset = 50;        // x offset for the mesh, try 50
int frameRate = 24;          // Change the rendering speed
// press C to start the displacement, V to stop; 1 to reduce displacement speed and 2 to increase

//path to pointcloud file (ASCII x y z R G B A, with x y z float and RGBA 8 bit). For RGB star/flare type distortion caused by the code NOT expecting alpha values, change the j+=7 to j+=6 at line 93.

string pattern = ""; //path to pointcloud file

//---------------------

vector<string> arr; // [x1 y1 z1 r1 g1 b1 a1 x2 y2 ....]
vector<ofColor> colArr; // r g b ints of points [r1 g1 b1 r2 g2 b2...]
vector<ofPoint> posArr; // x y z floats of points [x1 y1 z1 x2 y2 z2...]
vector<ofPoint> gridArr;
vector<ofPoint> dirArr;
vector<float> randArr;

ofFbo fbo;
ofPixels pix;

struct ColPos {
    ofColor color;
    ofPoint gridPos;
};

vector<ColPos> colPosArr;

bool wobble = 0; //from int
ofMesh mesh;
ofImage img;
int imageNumber = 0;

bool sortMe(ofColor & a, ofColor & b){
    if (a.getHue() < b.getHue()){
        return true;
    } else {
        return false;
    }
}

bool sortPos(ofVec3f & a, ofVec3f & b){
    if (a.length() < b.length()){
        return true;
    } else {
        return false;
    }
}

bool sortColPos(ColPos & a, ColPos & b){
    if (a.color.getHue() < b.color.getHue()){
        return true;
    } else {
        return false;
    }
}


ofPoint interpVector(ofPoint & a, ofPoint & b, float amt) {
    ofPoint newPos;
    newPos.x = ofLerp(a.x, b.x, amt);
    newPos.y = ofLerp(a.y, b.y, amt);
    newPos.z = ofLerp(a.z, b.z, amt);
    return newPos;
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(frameRate);
    
    //reads file line by line as strings into arr; arr.size = # points * 7
    string line;
    ifstream myfile (pattern);
    if (myfile.is_open())
    {
        while(getline(myfile,line, ' '))
        {
            arr.push_back(line);
        }
        arr.pop_back(); //deletes last rogue entry
        myfile.close();
        
        //load pos floats, RGB ints of points into arrays posArr and colArr
        for(int j = 0; j < arr.size(); j+=7) { //should be 7 if pointcloud is RGBA or 6 if RGB (or if you want flare/star distortion)
            posArr.push_back(ofVec3f(atof(arr[j].c_str()), -1*atof(arr[j+2].c_str()),  -1*atof( (arr[j+1].c_str())))); //swap y, z for -z, -y to match z-up y-forward orientation out of Blender. Here we have x+ right, y+ down, z+ backwards (out of screen)
            colArr.push_back(ofColor(atoi(arr[j+3].c_str()), atoi(arr[j+4].c_str()), atoi(arr[j+5].c_str())));
        }
        
        //ofSort(posArr, sortPos);
//                for(int j = 0; j < colArr.size(); j+=1) {
//                    cout << colArr[j];
//                    cout << "\n";
//                }
        
        cout << "Size of arr: " << arr.size();
        cout << "\n";
        cout << "Number of points: " << posArr.size();
        cout << "\n";
        
        img.allocate(ofGetWidth(), ofGetHeight(), OF_IMAGE_COLOR); // check these against posArr size!!!
        int k = 0;
        while ( k < img.getPixels().size() ) {//
        //while ( k < 10 ) {//
           // cout << "posarray " << posArr[k].x;   cout << "\n";
            img.getPixels()[k]   = colArr[k].r;
            img.getPixels()[k+1] = colArr[k].g;
            img.getPixels()[k+2] = colArr[k].b;
        //   img.getPixels()[k]   = abs(posArr[k].x)*2.0; why?
        //   img.getPixels()[k+1] = abs(posArr[k].y)*2.0;
          // img.getPixels()[k+2] = abs(posArr[k].z)*2.0;
           // img.getPixels()[k]   = posArr[k].x; why?
         //  img.getPixels()[k+1] = posArr[k].y;
           // img.getPixels()[k+2] = posArr[k].z;
            k+=3;
        }
        img.update();
    }
    
    else cout << "Unable to open file";
    
    mesh.setMode(OF_PRIMITIVE_POINTS);
    
    // scale and translate vertex
    for(int j = 0; j < posArr.size(); j+=1) {
        posArr[j] = ((posArr[j]*scalingFactor+ofVec3f(xoffset,yoffset,zoffset)));
    }
    
    for(int j = 0; j < colArr.size(); j+=1) {
        mesh.addVertex((posArr[j]));
        mesh.addColor(colArr[j]);
    }
    

    float totPoints = posArr.size();
    float xquadro = totPoints / 1.778f; // 2.387f for 2048x858 1.778f 1920x1080
    float x = sqrt(xquadro);
    
    int xwidth = x*1.778;//aspect ratio
    int xheight = x;
    
    cout << "grid width: " << xwidth << "\n";
    cout << "grid height: " << xheight << "\n";
    
    // create grid (display aspect ratio, size = point count, pre-pop with x'y'  coords)
    for(int i = 0; i < xwidth; i++) {
        for(int j = 0; j < xheight; j++) {
            gridArr.push_back(ofPoint(i-(xwidth)/2, j-(xheight)/2, 0));
        }
    }
    
    vector<ofColor> newColArr = colArr;
    ofSort(newColArr, sortMe);
    
    // init dir vector and rand vector
    for(int i = 0; i < gridArr.size(); i++) {
        dirArr.push_back(ofPoint(1,1,1));
        randArr.push_back(ofRandom(1.0)*2.0+0.5);
       // randArr.push_back(ofRandom(1.0)*3.0 + 2.5);
        
        // set colpos
        ColPos var;
        var.color = newColArr[i];
        var.gridPos = gridArr[i];
        colPosArr.push_back(var);
    }
    
    ofSort(colPosArr, sortColPos);
    for(int i = 0; i < gridArr.size(); i++) {
        gridArr[i] = colPosArr[i].gridPos;
    }
    
    
   // img.save("posArr.png");
    fbo.allocate(1920,1080,GL_RGB32F_ARB); //aspect ratio 570x1024 inf obj native
}

//--------------------------------------------------------------
void ofApp::update(){
    
    if(wobble == 1) {
        for(int i = 0; i < gridArr.size(); i++) {
           ofPoint dir =  gridArr[i] - posArr[i];
          if((gridArr[i].distance(posArr[i])) > 2.5) {
                dirArr[i] = dir.normalize()*randArr[i];
                posArr[i] -= dirArr[i]*speed; // explode to black
                //posArr[i] += dirArr[i]*speed;
            }
        }
        
        for(int i = 0; i < posArr.size(); i++) {
            mesh.setVertex(i, posArr[i]);
        }
    } else if(wobble == 0) {
        for(int i = 0; i < posArr.size(); i++) {
          // float rN = 1 + 0.0001 * (50-(rand()%100)+1);
           // float rN1 = 1 + 0.001 * (50-(rand()%100)+1);
          //  float rN2 = 1 + 0.001 * (50-(rand()%100)+1);
           // posArr[i] =  (posArr[i]* rN);
            mesh.setVertex(i, posArr[i]);
        }
    }
    
    imageNumber++;
}

//--------------------------------------------------------------
void ofApp::draw(){
    // background color
    //ofSetBackgroundColor(ofColor(163,0,128, 0));
    ofSetBackgroundColor(ofColor(255));

    img.draw(0,0);
   
    fbo.begin(); // these 3 lines clear any artefacts from GPU memory
    ofClear(0,0,0,255);
    fbo.end();
    
    fbo.begin();
    ofPushMatrix();
    
    float a_phase= 0.73 + 0.49*abs(cos(10*imageNumber/163.));//*cos(3.14159*imageNumber/2117.);//tan function for the *_phase variables will send them to infinity
    float b_phase= 0.77 + 0.51*abs(sin(1.14159*imageNumber/270.));
    float c_phase= 0.768+0.53*abs(sin(6.14159*imageNumber/110.));
    float rN = 1 + 0.005 * (50-(rand()%100)+1);
    float rN1 = 1 + 0.0049 * (50-(rand()%100)+1);
    float rN2 = 1 + 0.0051 * (50-(rand()%100)+1);
    
    ofTranslate(ofGetWidth()/2, ofGetHeight()/2, 0);\
    //ofRotateYRad(ofGetFrameNum()/((5.+(0.1*a_phase))*rotationSpeed) );
    //ofRotateZRad(ofGetFrameNum()/((5.+(0.1*a_phase))*rotationSpeed) );
    //ofRotateXRad(ofGetFrameNum()/((5.+(0.1*a_phase))*rotationSpeed) );
    //ofRotateXRad(ofGetFrameNum()/ ((3+(0.7*b_phase))*rotationSpeed) );
    //ofRotateZRad(ofGetFrameNum()/  ((2+(0.8*c_phase))*rotationSpeed) );
      ofRotateYRad(ofGetFrameNum()/(2*rotationSpeed));
      ofRotateXRad(ofGetFrameNum()/(4*rotationSpeed));
    ofRotateZRad(ofGetFrameNum()/  (10*rotationSpeed));
    //breathe (choose one of below)
   ofScale(abs(sqrt(rN*rN2) * b_phase), abs(sqrt(rN1*rN)* c_phase), abs(sqrt(rN1*rN2)* a_phase)); //breathe, abs value prevents eversion
    ofScale(abs(b_phase), abs(c_phase), abs(a_phase));
   //jitter below
    //ofScale(0.5*rN+rN1, 0.48*rN1+rN2, 0.51*rN2+rN); //jitter
    
    mesh.draw();
    ofPopMatrix();
    fbo.end();
    fbo.readToPixels(pix);
    fbo.draw(0,0);
    
    //pngs will be saved to ../cloudBloomer/bin/data
    stringstream s;
    string line = "output_";
    string line2 = ".png";
    s << line << imageNumber << line2;
    string result = s.str();
    //ofSaveImage(pix, result);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key == 'c') wobble = 1;
    else if(key == 'v') wobble = 0;
    else if(key == '2') speed = speed * 1.1;
    else if(key == '1') speed = speed / 1.1;
    else if(key == '5') rotationSpeed = rotationSpeed / 1.1;
    else if(key == '4') rotationSpeed = rotationSpeed * 1.1;
    else if(key == '6') rotationSpeed = rotationSpeed * -1.;
  //  else if(key == 'x') xoffset = xoffset - 25; // this needs another try
  //  else if(key == 'X') xoffset = xoffset + 25;
  //  else if(key == 'y') yoffset = yoffset - 25;
  //  else if(key == 'Y') yoffset = yoffset + 25;
  //  else if(key == 'z') zoffset = zoffset - 25;
  //  else if(key == 'Z') zoffset = zoffset + 25;
 //   else if(key == '-') scalingFactor = scalingFactor / 1.25;
 //   else if(key == '+') scalingFactor = scalingFactor * 1.25;
}
//--------------------------------------------------------------
void ofApp::keyReleased(int key){
}
//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
}
//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
}
//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
}
//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}
//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
}
//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
}
//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
}
//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
}
//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
}

