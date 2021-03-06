#include <algorithm>
#include <functional>
#include <utility>
#include <vector>
#include <iostream>

#include "caffe/layers/argmax_layer_all.hpp"

namespace caffe {

template <typename Dtype>
void ArgMaxAllLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  const ArgMaxParameter& argmax_param = this->layer_param_.argmax_param();
  out_max_val_ = argmax_param.out_max_val();
  top_k_ = argmax_param.top_k();
  has_axis_ = argmax_param.has_axis();
  CHECK_GE(top_k_, 1) << "top k must not be less than 1.";
  if (has_axis_) {
    axis_ = bottom[0]->CanonicalAxisIndex(argmax_param.axis());
    CHECK_GE(axis_, 0) << "axis must not be less than 0.";
    CHECK_LE(axis_, bottom[0]->num_axes()) <<
      "axis must be less than or equal to the number of axis.";
    //CHECK_LE(top_k_, bottom[0]->shape(axis_))
      //<< "top_k must be less than or equal to the dimension of the axis.";
  } else {
    CHECK_LE(top_k_, bottom[0]->count(1))
      << "top_k must be less than or equal to"
        " the dimension of the flattened bottom blob per instance.";
  }
}

template <typename Dtype>
void ArgMaxAllLayer<Dtype>::Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  int num_top_axes = bottom[0]->num_axes();
//  std::cout << top_k_ << " pre\n";
//  top_k_new = 1000;//(const int) bottom[0]->shape(3);
//  std::cout << top_k_new << " post\n";
  top_k_ = 0;
  const Dtype* bottom_data_obj = bottom[0]->cpu_data();
  const Dtype* bottom_data_noObj = bottom[1]->cpu_data();
  for (int i = 0; i < bottom[1]->shape(3); i++){
    top_k_ += (bottom_data_obj[i]>bottom_data_noObj[i]);
  }
  top_k_new = std::max((const int) top_k_, (const int) 1000);
  if ( num_top_axes < 3 ) num_top_axes = 3;
  std::vector<int> shape(num_top_axes, 1);
  if (has_axis_) {
    // Produces max_ind or max_val per axis
    shape = bottom[0]->shape();
    shape[axis_] = top_k_new;
  } else {
    shape[0] = bottom[0]->shape(0);
    // Produces max_ind
    shape[2] = top_k_new;
    if (out_max_val_) {
      // Produces max_ind and max_val
      shape[1] = 2;
    }
  }
  top[0]->Reshape(shape);
//  std::cout << top[0]->shape(0) << " " << top[0]->shape(1) << " " << top[0]->shape(2) << " " << top[0]->shape(3) << " " << " post\n";
}

template <typename Dtype>
void ArgMaxAllLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    const vector<Blob<Dtype>*>& top) {
  const Dtype* bottom_data = bottom[0]->cpu_data();
  Dtype* top_data = top[0]->mutable_cpu_data();
  int dim, axis_dist;
  if (has_axis_) {
    dim = bottom[0]->shape(axis_);
    // Distance between values of axis in blob
    axis_dist = bottom[0]->count(axis_) / dim;
  } else {
    dim = bottom[0]->count(1);
    axis_dist = 1;
  }
  int num = bottom[0]->count() / dim;
  std::vector<std::pair<Dtype, int> > bottom_data_vector(dim);
  for (int i = 0; i < num; ++i) {
    for (int j = 0; j < dim; ++j) {
      bottom_data_vector[j] = std::make_pair(
        bottom_data[(i / axis_dist * dim + j) * axis_dist + i % axis_dist], j);
    }
    std::partial_sort(
        bottom_data_vector.begin(), bottom_data_vector.begin() + top_k_new,
        bottom_data_vector.end(), std::greater<std::pair<Dtype, int> >());
    for (int j = 0; j < top_k_new; ++j) {
      if (out_max_val_) {
        if (has_axis_) {
          // Produces max_val per axis
          top_data[(i / axis_dist * top_k_new + j) * axis_dist + i % axis_dist]
            = bottom_data_vector[j].first;
        } else {
          // Produces max_ind and max_val
          top_data[2 * i * top_k_new + j] = bottom_data_vector[j].second;
          top_data[2 * i * top_k_new + top_k_new + j] = bottom_data_vector[j].first;
        }
      } else {
        // Produces max_ind per axis
        top_data[(i / axis_dist * top_k_new + j) * axis_dist + i % axis_dist]
          = bottom_data_vector[j].second;
      }
    }
  }
}


template <typename Dtype>
void ArgMaxAllLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {
//  bottom[0]->ShareDiff(*top[0]);
}

INSTANTIATE_CLASS(ArgMaxAllLayer);
REGISTER_LAYER_CLASS(ArgMaxAll);

}  // namespace caffe
