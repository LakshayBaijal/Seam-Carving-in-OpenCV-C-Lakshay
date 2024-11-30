#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>  // For dynamic memory allocation

using namespace cv;
using namespace std;

// Calculate energy map of image
Mat ComputeEnergyMap(const Mat &image) {
    Mat grayImage, gradX, gradY, absGradX, absGradY, energymap;

    cvtColor(image, grayImage, COLOR_BGR2GRAY);

    Sobel(grayImage, gradX, CV_16S, 1, 0, 3);
    Sobel(grayImage, gradY, CV_16S, 0, 1, 3);

    convertScaleAbs(gradX, absGradX);
    convertScaleAbs(gradY, absGradY);

    addWeighted(absGradX, 0.5, absGradY, 0.5, 0, energymap);

    return energymap;
}

// Finding vertical seam with minimum energy
void FindVerticalSeam(const Mat &energymap, int *seam) {
    int rows = energymap.rows;
    int cols = energymap.cols;

    float **energy = (float **)malloc(rows * sizeof(float *));
    for (int i = 0; i < rows; ++i) {
        energy[i] = (float *)malloc(cols * sizeof(float));
    }

    // Initialise first row for path initiation
    for (int i = 0; i < cols; ++i) {
        energy[0][i] = energymap.at<uchar>(0, i);
    }


    // Compute the minimum energy path
    float minenergy;
    for (int i = 1; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            minenergy = energy[i - 1][j];
            if (j > 0) 
            {
                minenergy = min(minenergy, energy[i - 1][j - 1]);
            }
            if (j < cols - 1) 
            {
                minenergy = min(minenergy, energy[i - 1][j + 1]);
            }
            energy[i][j] = minenergy + energymap.at<uchar>(i, j);
        }
    }

    // Find the minimum energy seam
    int minpoint = 0;
    for (int i = 1; i < cols; ++i) 
    {
        minenergy = energy[rows - 1][0];

        if (energy[rows - 1][i] < minenergy) {
            minenergy = energy[rows - 1][i];
            minpoint = i;
        }
    }

    seam[rows - 1] = minpoint;

    // Backtrack to get seam path
    for (int i = rows - 2; i >= 0; --i) {
        int prevcolumn = seam[i + 1];
        int mincolumn = seam[i + 1];

        if (prevcolumn > 0 && energy[i][mincolumn - 1] < energy[i][mincolumn]) {
            mincolumn = prevcolumn - 1;
        }
        if (prevcolumn < cols - 1 && energy[i][prevcolumn + 1] < energy[i][mincolumn]) {
            mincolumn = prevcolumn + 1;
        }
        seam[i] = mincolumn;
    }

    for (int i = 0; i < rows; ++i) {
        free(energy[i]);
    }
    free(energy);
}

// Remove vertical seam from image
void RemoveVerticalSeam(Mat &image, int *seam) {
    int rows = image.rows;
    int cols = image.cols;

    for (int i = 0; i < rows; ++i) {
        for (int j = seam[i]; j < cols - 1; ++j) {
            image.at<Vec3b>(i, j) = image.at<Vec3b>(i, j + 1);
        }
    }
    image = image(Rect(0, 0, cols - 1, rows));
}

// Remove horizontal seam from image
void RemoveHorizontalSeam(Mat &image, int *seam) {
    int rows = image.rows;
    int cols = image.cols;

    for (int i = 0; i < cols; ++i) {
        for (int j = seam[i]; j < rows - 1; ++j) {
            image.at<Vec3b>(j, i) = image.at<Vec3b>(j + 1, i);
        }
    }
    image = image(Rect(0, 0, cols, rows - 1));
}

// Highlight seam in image
void HighlightSeam(Mat &image, int *seam) 
{
    for (int i = 0; i < image.rows; ++i) 
    {
        image.at<Vec3b>(i, seam[i]) = Vec3b(255, 255, 255);  
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        cout << "Execute in terminal by: " << argv[0] << " <image_path> <width> <height>" << endl;
        return -1;
    }

    string imagePath = argv[1];
    Mat image = imread(imagePath);
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);

    if (image.empty()) {
        cout << "Error loading image: " << imagePath << endl;
        return -1;
    }

    if (width <= 0 || width >= image.cols) {
        cout << "Invalid width. image width is: " << image.cols << endl;
        return -1;
    }

    if (height <= 0 || height >= image.rows) {
        cout << "Invalid height. image height is: " << image.rows << endl;
        return -1;
    }

    int *seam = (int *)malloc(image.rows * sizeof(int));


    // Reduce width of the image
    while (image.cols > width) {
        Mat energyMap = ComputeEnergyMap(image);

        FindVerticalSeam(energyMap, seam);
        HighlightSeam(image, seam);

        imshow("Lakshay Seam Carving Reducing Width", image);
        waitKey(50);

        RemoveVerticalSeam(image, seam);
    }

    imwrite("reduced_width_image.jpg", image);
    image = imread("reduced_width_image.jpg");

    seam = (int *)realloc(seam, image.cols * sizeof(int));

    // Reduce height of the image
    while (image.rows > height) {
        Mat transposedImage = image.t();

        Mat energyMap = ComputeEnergyMap(transposedImage);

        FindVerticalSeam(energyMap, seam);
        HighlightSeam(transposedImage, seam);

        imshow("Lakshay Seam Carving Reducing Height", transposedImage.t());
        waitKey(50);

        RemoveHorizontalSeam(image, seam);
    }

    imshow("Lakshay Seam Carving Final Image", image);
    waitKey(0);

    imwrite("../sample_output/final_output.jpeg", image);
    cout << "Final image saved as final_output.jpeg with width " << width << " and height " << height << endl;

    // Free allocated memory
    free(seam);

    return 0;
}
