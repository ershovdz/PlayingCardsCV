/*
 * Copyright (C) 2011, Mathieu Labbe - IntRoLab - Universite de Sherbrooke
 */

#include "AddObjectDialog.h"
#include "ui_addObjectDialog.h"
#include "ObjWidget.h"
#include "KeypointItem.h"
#include "Camera.h"
#include "qtipl.h"
#include "Settings.h"

#include <stdio.h>

#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsPixmapItem>
#include <QtGui/QMessageBox>

#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

AddObjectDialog::AddObjectDialog(Camera * camera, const IplImage * image, bool mirrorView, QWidget * parent, Qt::WindowFlags f) :
		QDialog(parent, f),
		camera_(camera),
		object_(0),
		cvImage_(0)
{
	ui_ = new Ui_addObjectDialog();
	ui_->setupUi(this);

	connect(ui_->pushButton_cancel, SIGNAL(clicked()), this, SLOT(cancel()));
	connect(ui_->pushButton_back, SIGNAL(clicked()), this, SLOT(back()));
	connect(ui_->pushButton_next, SIGNAL(clicked()), this, SLOT(next()));
	connect(ui_->pushButton_takePicture, SIGNAL(clicked()), this, SLOT(takePicture()));
	connect(ui_->comboBox_selection, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSelectionMode()));

	connect(ui_->cameraView, SIGNAL(selectionChanged()), this, SLOT(updateNextButton()));
	connect(ui_->cameraView, SIGNAL(roiChanged(const QRect &)), this, SLOT(updateNextButton(const QRect &)));
	ui_->cameraView->setMirrorView(mirrorView);

	if((camera_ && camera_->isRunning()) || !image)
	{
		this->setState(kTakePicture);
	}
	else if(image)
	{
		cv::Mat img(image);
		update(img);
		this->setState(kSelectFeatures);
	}
}

AddObjectDialog::~AddObjectDialog()
{
	if(cvImage_)
	{
		cvReleaseImage(&cvImage_);
	}
	if(object_)
	{
		delete object_;
	}
	delete ui_;
}

void AddObjectDialog::closeEvent(QCloseEvent* event)
{
	if(camera_)
	{
		disconnect(camera_, SIGNAL(imageReceived(const cv::Mat &)), this, SLOT(update(const cv::Mat &)));
	}
	QDialog::closeEvent(event);
}

void AddObjectDialog::next()
{
	setState(state_+1);
}
void AddObjectDialog::back()
{
	setState(state_-1);
}
void AddObjectDialog::cancel()
{
	this->reject();
}
void AddObjectDialog::takePicture()
{
	next();
}

void AddObjectDialog::updateNextButton()
{
	updateNextButton(QRect());
}

void AddObjectDialog::updateNextButton(const QRect & rect)
{
	roi_ = rect;
	if(state_ == kSelectFeatures)
	{
		if(ui_->comboBox_selection->currentIndex() == 1)
		{
			if(ui_->cameraView->selectedItems().size() > 0)
			{
				ui_->pushButton_next->setEnabled(true);
			}
			else
			{
				ui_->pushButton_next->setEnabled(false);
			}
		}
		else
		{
			if(roi_.isNull())
			{
				ui_->pushButton_next->setEnabled(false);
			}
			else
			{
				ui_->pushButton_next->setEnabled(true);
			}
		}
	}
}

void AddObjectDialog::changeSelectionMode()
{
	this->setState(kSelectFeatures);
}

void AddObjectDialog::setState(int state)
{
	state_ = state;
	if(state == kTakePicture)
	{
		ui_->pushButton_cancel->setEnabled(true);
		ui_->pushButton_back->setEnabled(false);
		ui_->pushButton_next->setEnabled(false);
		ui_->pushButton_takePicture->setEnabled(true);
		ui_->label_instruction->setText(tr("Place the object in front of the camera and click \"Take picture\"."));
		ui_->pushButton_next->setText(tr("Next"));
		ui_->cameraView->setVisible(true);
		ui_->cameraView->clearRoiSelection();
		ui_->objectView->setVisible(false);
		ui_->cameraView->setGraphicsViewMode(false);
		ui_->comboBox_selection->setVisible(false);
		if(!camera_ || !camera_->start())
		{
			QMessageBox::critical(this, tr("Camera error"), tr("Camera is not started!"));
			ui_->pushButton_takePicture->setEnabled(false);
		}
		else
		{
			connect(camera_, SIGNAL(imageReceived(const cv::Mat &)), this, SLOT(update(const cv::Mat &)));
		}
	}
	else if(state == kSelectFeatures)
	{
		if(camera_)
		{
			disconnect(camera_, SIGNAL(imageReceived(const cv::Mat &)), this, SLOT(update(const cv::Mat &)));
			camera_->pause();
		}

		ui_->pushButton_cancel->setEnabled(true);
		ui_->pushButton_back->setEnabled(camera_);
		ui_->pushButton_next->setEnabled(false);
		ui_->pushButton_takePicture->setEnabled(false);
		ui_->pushButton_next->setText(tr("Next"));
		ui_->cameraView->setVisible(true);
		ui_->cameraView->clearRoiSelection();
		ui_->objectView->setVisible(false);
		ui_->comboBox_selection->setVisible(true);

		if(ui_->comboBox_selection->currentIndex() == 1)
		{
			ui_->label_instruction->setText(tr("Select features representing the object."));
			ui_->cameraView->setGraphicsViewMode(true);
		}
		else
		{
			ui_->label_instruction->setText(tr("Select region representing the object."));
			ui_->cameraView->setGraphicsViewMode(false);
		}
		updateNextButton(QRect());
	}
	else if(state == kVerifySelection)
	{
		if(camera_)
		{
			disconnect(camera_, SIGNAL(imageReceived(const cv::Mat &)), this, SLOT(update(const cv::Mat &)));
			camera_->pause();
		}

		ui_->pushButton_cancel->setEnabled(true);
		ui_->pushButton_back->setEnabled(true);
		ui_->pushButton_takePicture->setEnabled(false);
		ui_->pushButton_next->setText(tr("End"));
		ui_->cameraView->setVisible(true);
		ui_->objectView->setVisible(true);
		ui_->objectView->setMirrorView(ui_->cameraView->isMirrorView());
		ui_->objectView->setSizedFeatures(ui_->cameraView->isSizedFeatures());
		ui_->comboBox_selection->setVisible(false);
		if(ui_->comboBox_selection->currentIndex() == 1)
		{
			ui_->cameraView->setGraphicsViewMode(true);
		}
		else
		{
			ui_->cameraView->setGraphicsViewMode(false);
		}

		std::vector<cv::KeyPoint> selectedKeypoints = ui_->cameraView->selectedKeypoints();

		// Select keypoints
		if(cvImage_ &&
				((ui_->comboBox_selection->currentIndex() == 1 && selectedKeypoints.size()) ||
				 (ui_->comboBox_selection->currentIndex() == 0 && !roi_.isNull())))
		{
			CvRect roi;
			if(ui_->comboBox_selection->currentIndex() == 1)
			{
				roi = computeROI(selectedKeypoints);
			}
			else
			{
				roi.x = roi_.x();
				roi.y = roi_.y();
				roi.width = roi_.width();
				roi.height = roi_.height();
			}
			cvSetImageROI(cvImage_, roi);

			if(ui_->comboBox_selection->currentIndex() == 1)
			{
				if(roi.x != 0 || roi.y != 0)
				{
					for(unsigned int i=0; i<selectedKeypoints.size(); ++i)
					{
						selectedKeypoints.at(i).pt.x -= roi.x;
						selectedKeypoints.at(i).pt.y -= roi.y;
					}
				}
			}
			else
			{
				// Extract keypoints
				selectedKeypoints.clear();
				cv::FeatureDetector * detector = Settings::createFeaturesDetector();
				detector->detect(cvImage_, selectedKeypoints);
				delete detector;
			}
			ui_->objectView->setData(selectedKeypoints, cv::Mat(), cvImage_, Settings::currentDetectorType(), "");
			ui_->objectView->setMinimumSize(roi.width, roi.height);
			ui_->objectView->update();
			cvResetImageROI(cvImage_);
			ui_->pushButton_next->setEnabled(true);
		}
		else
		{
			printf("Please select items\n");
			ui_->pushButton_next->setEnabled(false);
		}
		ui_->label_instruction->setText(tr("Selection : %1 features").arg(selectedKeypoints.size()));
	}
	else if(state == kClosing)
	{
		std::vector<cv::KeyPoint> keypoints = ui_->objectView->keypoints();
		if((ui_->comboBox_selection->currentIndex() == 1 && keypoints.size()) ||
		   (ui_->comboBox_selection->currentIndex() == 0 && !roi_.isNull()))
		{
			cv::Mat descriptors;
			if(keypoints.size())
			{
				// Extract descriptors
				cv::DescriptorExtractor * extractor = Settings::createDescriptorsExtractor();
				extractor->compute(ui_->objectView->iplImage(), keypoints, descriptors);
				delete extractor;

				if(keypoints.size() != (unsigned int)descriptors.rows)
				{
					printf("ERROR : keypoints=%d != descriptors=%d\n", (int)keypoints.size(), descriptors.rows);
				}
			}

			if(object_)
			{
				delete object_;
				object_ = 0;
			}
			object_ = new ObjWidget(0, keypoints, descriptors, ui_->objectView->iplImage(), Settings::currentDetectorType(), Settings::currentDescriptorType());

			this->accept();
		}
	}
}

void AddObjectDialog::update(const cv::Mat & image)
{
	if(cvImage_)
	{
		cvReleaseImage(&cvImage_);
		cvImage_ = 0;
	}
	IplImage iplImage = image;
	cvImage_ = cvCloneImage(& iplImage);
	if(cvImage_)
	{
		// convert to grayscale
		if(cvImage_->nChannels != 1 || cvImage_->depth != IPL_DEPTH_8U)
		{
			IplImage * imageGrayScale = cvCreateImage(cvSize(cvImage_->width, cvImage_->height), IPL_DEPTH_8U, 1);
			cvCvtColor(cvImage_, imageGrayScale, CV_BGR2GRAY);
			cvReleaseImage(&cvImage_);
			cvImage_ = imageGrayScale;
		}

		// Extract keypoints
		cv::FeatureDetector * detector = Settings::createFeaturesDetector();
		cv::vector<cv::KeyPoint> keypoints;
		detector->detect(cvImage_, keypoints);
		delete detector;

		ui_->cameraView->setData(keypoints, cv::Mat(), cvImage_, Settings::currentDetectorType(), "");
		ui_->cameraView->update();
	}
}

CvRect AddObjectDialog::computeROI(const std::vector<cv::KeyPoint> & kpts)
{
	CvRect roi = cvRect(0,0,0,0);
	int x1=0,x2=0,h1=0,h2=0;
	for(unsigned int i=0; i<kpts.size(); ++i)
	{
		float radius = kpts.at(i).size / 2;
		if(i==0)
		{
			x1 = int(kpts.at(i).pt.x - radius);
			x2 = int(kpts.at(i).pt.x + radius);
			h1 = int(kpts.at(i).pt.y - radius);
			h2 = int(kpts.at(i).pt.y + radius);
		}
		else
		{
			if(x1 > int(kpts.at(i).pt.x - radius))
			{
				x1 = int(kpts.at(i).pt.x - radius);
			}
			else if(x2 < int(kpts.at(i).pt.x + radius))
			{
				x2 = int(kpts.at(i).pt.x + radius);
			}
			if(h1 > int(kpts.at(i).pt.y - radius))
			{
				h1 = int(kpts.at(i).pt.y - radius);
			}
			else if(h2 < int(kpts.at(i).pt.y + radius))
			{
				h2 = int(kpts.at(i).pt.y + radius);
			}
		}
		roi.x = x1;
		roi.y = h1;
		roi.width = x2-x1;
		roi.height = h2-h1;
		//printf("ptx=%d, pty=%d\n", (int)kpts.at(i).pt.x, (int)kpts.at(i).pt.y);
		//printf("x=%d, y=%d, w=%d, h=%d\n", roi.x, roi.y, roi.width, roi.height);
	}

	return roi;
}
