#include "reassembler.hh"


using namespace std;

Sub::Sub(uint64_t index, const std::string& data, bool is_last_substring) : index(index), data(data), is_last_substring(is_last_substring) {}

bool overlap(const Sub& a, const Sub& b) {
    return a.index + a.data.length() >= b.index;
}

// Function to merge two overlapping pairs
Sub merge(const Sub& a, const Sub& b) {
    string newData;
    bool isLast;
    if(a.index + a.data.length() > b.index + b.data.length()){
      newData = a.data;
      isLast = a.is_last_substring;
    }else{
      newData = a.data + b.data.substr(a.index + a.data.length() - b.index);
      isLast = b.is_last_substring;
    }
    return Sub(a.index, newData, isLast);
}

bool compareSubs(const Sub& a, const Sub& b) {
    return a.index < b.index;
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring)
{
  uint64_t store_endIdx = ack_index + output_.available_capacity();
  if(store_endIdx <= first_index){
    return;
  }
  uint64_t  store_len = min(store_endIdx - first_index, data.length());
  Sub newSub(first_index, data.substr(0, store_len), is_last_substring);

  // auto insertedPos = subList.begin();
  // for(; insertedPos != subList.end(); insertedPos++){
  //   if(insertedPos->index >= first_index){
  //     break;
  //   }
  // }
  // subList.insert(insertedPos, newSub);

  subList.push_back(newSub);
  subList.sort(compareSubs);

  // Iterate through the list and merge overlapping pairs
  auto it = subList.begin();
  while (it != subList.end()) {
      auto nextElem = next(it);

      if (nextElem != subList.end() && overlap(*it, *nextElem)) {
          // Merge overlapping pairs
          *it = merge(*it, *nextElem);
          subList.erase(nextElem);
      } else {
          ++it;
      }
  }

  if(!subList.empty() && subList.front().index <= ack_index){
    const Sub& firstElement = subList.front();
    if(firstElement.index + firstElement.data.length() >= ack_index){
      // int len = min(firstElement.data.length() - (ack_index - firstElement.index), output.available_capacity());
      // cout << "available_capacity: " << output.available_capacity() << endl;
      // cout << "ack_index " << ack_index << endl;
      // cout << "endIdx " << endIdx << endl;
      string writedStr = firstElement.data.substr(ack_index - firstElement.index);
      output_.push(writedStr);
      // cout << "push string: " << writedStr << endl;
      if(firstElement.is_last_substring){
        output_.close();
      }
      ack_index += writedStr.length();
    }
    subList.pop_front();
  }

  stored_bytes = 0;
  for(auto it2 = subList.begin(); it2 != subList.end(); it2++){
    stored_bytes += it2->data.length();
  }

  
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return stored_bytes;
}
