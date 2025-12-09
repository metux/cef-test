
#include "../cefhelper_priv.h"
#include "include/cef_client.h"

__attribute__((visibility("default"), used))
class CreateBrowserTask: public CefTask {
public:
    CreateBrowserTask(CefClientRef client, uint32_t parent_xid,
                      int width, int height, std::string url)
        : _client(client), _parent_xid(parent_xid), _width(width),
          _height(height), _url(url)
    {
        fprintf(stderr, "CreateBrowserTask() constructor\n");
    }

    static CefWindowInfo make_window_info(uint32_t parent_xid, int width, int height) {
        fprintf(stderr, "CreateBrowserTask() make_window_info()\n");
        CefWindowInfo wi;
        wi.SetAsChild(
            (CefWindowHandle)parent_xid,
            CefRect(0, 0, width, height));
#ifdef USE_ALLOY
        wi.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
#endif
        return wi;
    }

    void Execute() override {
        fprintf(stderr, "CreateBrowserTask()::Execute()\n");
        CefBrowserHost::CreateBrowser(make_window_info(_parent_xid, _width, _height),
                                      _client,
                                      _url,
                                      CefBrowserSettings(),
                                      nullptr,
                                      nullptr);
    }

private:
    CefClientRef _client;
    uint32_t _parent_xid;
    int _width;
    int _height;
    std::string _url;

 public:                                          
  __attribute__((visibility("default")))          
  void AddRef() const override {                  
    fprintf(stderr, "CreateBrowserTask::AddRef()\n"); 
    ref_count_.AddRef();                          
  }                                               
  __attribute__((visibility("default"), used))    
  bool Release() const override {                 
    fprintf(stderr, "CreateBrowserTask::Release()\n"); 
    if (ref_count_.Release()) {                   
      fprintf(stderr, "CreateBrowserTask::Release() deleting\n");
      delete static_cast<const CreateBrowserTask*>(this); 
      return true;                                
    }                                             
    return false;                                 
  }                                               
  __attribute__((visibility("default"), used))    
  bool HasOneRef() const override {               
    fprintf(stderr, "CreateBrowserTask::HasOneRef()\n"); 
    return ref_count_.HasOneRef();                
  }                                               
  __attribute__((visibility("default"), used))    
  bool HasAtLeastOneRef() const override {        
    fprintf(stderr, "CreateBrowserTask::HasAtLeastOneRef()\n"); 
    return ref_count_.HasAtLeastOneRef();         
  }                                               
                                                  
 private:                                         
  CefRefCount ref_count_;
};

void taskCreate(CefClientRef client, uint32_t parent_xid,
                int width, int height, std::string url) {
    CefRefPtr<CefTask> t = new CreateBrowserTask(client, parent_xid, width, height, url);
    CefPostTask(TID_UI, t);
}
