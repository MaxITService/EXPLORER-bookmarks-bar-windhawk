#pragma once
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Streams.h>

// Contract doubles for WinRT drag arguments and asynchronous item reads. The
// production callbacks, insertion algorithm and real XAML controls are used.
namespace drop_fixture {
namespace wf = winrt::Windows::Foundation;
namespace wfc = wf::Collections;
namespace ws = winrt::Windows::Storage;
using Items = wfc::IVectorView<ws::IStorageItem>;
using Read = wf::IAsyncOperation<Items>;
using Completion = wf::AsyncOperationCompletedHandler<Items>;

struct Item : winrt::implements<Item, ws::IStorageItem> {
    std::wstring path;
    bool folder;
    bool failPath = false;
    Item(std::wstring value, bool isFolder = true) : path(std::move(value)), folder(isFolder) {}
    winrt::hstring Path() { if (failPath) throw winrt::hresult_error(E_FAIL); return winrt::hstring(path); }
    bool IsOfType(ws::StorageItemTypes type) { return type == (folder ? ws::StorageItemTypes::Folder : ws::StorageItemTypes::File); }
    winrt::hstring Name() { return winrt::hstring(path); }
    ws::FileAttributes Attributes() { return ws::FileAttributes::Normal; }
    wf::DateTime DateCreated() { return {}; }
    wf::IAsyncAction RenameAsync(const winrt::hstring&) { throw winrt::hresult_not_implemented(); }
    wf::IAsyncAction RenameAsync(const winrt::hstring&, ws::NameCollisionOption) { throw winrt::hresult_not_implemented(); }
    wf::IAsyncAction DeleteAsync() { throw winrt::hresult_not_implemented(); }
    wf::IAsyncAction DeleteAsync(ws::StorageDeleteOption) { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<ws::FileProperties::BasicProperties> GetBasicPropertiesAsync() { throw winrt::hresult_not_implemented(); }
};

struct AsyncRead : winrt::implements<AsyncRead, Read, wf::IAsyncInfo> {
    Items items = winrt::single_threaded_vector<ws::IStorageItem>().GetView();
    Completion completion{nullptr};
    wf::AsyncStatus status = wf::AsyncStatus::Started;
    bool failRegistration = false;
    bool failResults = false;
    unsigned registrations = 0;
    unsigned resultReads = 0;
    unsigned callbacks = 0;
    void Completed(const Completion& handler) {
        ++registrations;
        if (failRegistration) throw winrt::hresult_error(E_FAIL);
        completion = handler;
        if (status != wf::AsyncStatus::Started) Deliver();
    }
    Completion Completed() { return completion; }
    Items GetResults() { ++resultReads; if (failResults) throw winrt::hresult_error(E_FAIL); return items; }
    uint32_t Id() { return 1; }
    wf::AsyncStatus Status() { return status; }
    winrt::hresult ErrorCode() { return status == wf::AsyncStatus::Error ? E_FAIL : S_OK; }
    void Cancel() { Finish(wf::AsyncStatus::Canceled); }
    void Close() {}
    void Deliver() {
        if (auto handler = std::exchange(completion, nullptr)) {
            ++callbacks;
            handler(get_strong().as<Read>(), status);
        }
    }
    void Finish(wf::AsyncStatus value = wf::AsyncStatus::Completed) {
        Require(status == wf::AsyncStatus::Started, "test async read completed twice");
        status = value;
        Deliver();
    }
    void SetItems(std::vector<ws::IStorageItem> values) {
        items = winrt::single_threaded_vector<ws::IStorageItem>(std::move(values)).GetView();
    }
};

struct DataView : winrt::implements<DataView, wdt::IDataPackageView> {
    Read read{nullptr};
    bool storageItems = true;
    bool failReadStart = false;
    unsigned reads = 0;
    unsigned formatQueries = 0;
    bool Contains(const winrt::hstring& format) { ++formatQueries; return storageItems && format == wdt::StandardDataFormats::StorageItems(); }
    Read GetStorageItemsAsync() { ++reads; if (failReadStart) throw winrt::hresult_error(E_FAIL); return read; }
    wdt::DataPackagePropertySetView Properties() { throw winrt::hresult_not_implemented(); }
    wdt::DataPackageOperation RequestedOperation() { return wdt::DataPackageOperation::Link; }
    void ReportOperationCompleted(wdt::DataPackageOperation) {}
    wfc::IVectorView<winrt::hstring> AvailableFormats() { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<wf::IInspectable> GetDataAsync(const winrt::hstring&) { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<winrt::hstring> GetTextAsync() { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<winrt::hstring> GetTextAsync(const winrt::hstring&) { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<wf::Uri> GetUriAsync() { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<winrt::hstring> GetHtmlFormatAsync() { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<wfc::IMapView<winrt::hstring, ws::Streams::RandomAccessStreamReference>> GetResourceMapAsync() { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<winrt::hstring> GetRtfAsync() { throw winrt::hresult_not_implemented(); }
    wf::IAsyncOperation<ws::Streams::RandomAccessStreamReference> GetBitmapAsync() { throw winrt::hresult_not_implemented(); }
};

struct DragUI : winrt::implements<DragUI, mux::IDragUIOverride> {
    winrt::hstring caption;
    bool captionVisible = false;
    bool contentVisible = false;
    bool glyphVisible = false;
    winrt::hstring Caption() { return caption; }
    void Caption(const winrt::hstring& value) { caption = value; }
    bool IsCaptionVisible() { return captionVisible; }
    void IsCaptionVisible(bool value) { captionVisible = value; }
    bool IsContentVisible() { return contentVisible; }
    void IsContentVisible(bool value) { contentVisible = value; }
    bool IsGlyphVisible() { return glyphVisible; }
    void IsGlyphVisible(bool value) { glyphVisible = value; }
    void Clear() { caption.clear(); captionVisible = contentVisible = glyphVisible = false; }
    void SetContentFromBitmapImage(const muxmi::BitmapImage&) { throw winrt::hresult_not_implemented(); }
    void SetContentFromBitmapImage(const muxmi::BitmapImage&, wf::Point) { throw winrt::hresult_not_implemented(); }
    void SetContentFromSoftwareBitmap(const winrt::Windows::Graphics::Imaging::SoftwareBitmap&) { throw winrt::hresult_not_implemented(); }
    void SetContentFromSoftwareBitmap(const winrt::Windows::Graphics::Imaging::SoftwareBitmap&, wf::Point) { throw winrt::hresult_not_implemented(); }
};

struct DragArgs : winrt::implements<DragArgs, mux::IDragEventArgs> {
    wdt::DataPackageView view{nullptr};
    wdt::DataPackage data{nullptr};
    winrt::com_ptr<DragUI> ui = winrt::make_self<DragUI>();
    wdt::DataPackageOperation allowed = wdt::DataPackageOperation::Link;
    wdt::DataPackageOperation accepted = wdt::DataPackageOperation::None;
    mux::UIElement strip{nullptr};
    wf::Point stripPoint{5, 5};
    wf::Point panelPoint{0, 5};
    unsigned deferrals = 0;
    bool handled = false;
    bool Handled() { return handled; }
    void Handled(bool value) { handled = value; }
    wdt::DataPackage Data() { return data; }
    void Data(const wdt::DataPackage& value) { data = value; }
    wdt::DataPackageView DataView() { return view; }
    mux::DragUIOverride DragUIOverride() { return ui.as<mux::DragUIOverride>(); }
    wdt::DragDrop::DragDropModifiers Modifiers() { return wdt::DragDrop::DragDropModifiers::None; }
    wdt::DataPackageOperation AllowedOperations() { return allowed; }
    wdt::DataPackageOperation AcceptedOperation() { return accepted; }
    void AcceptedOperation(wdt::DataPackageOperation value) { accepted = value; }
    mux::DragOperationDeferral GetDeferral() { ++deferrals; throw winrt::hresult_not_implemented(); }
    wf::Point GetPosition(const mux::UIElement& relativeTo) { return relativeTo == strip ? stripPoint : panelPoint; }
};

struct Request {
    winrt::com_ptr<AsyncRead> read = winrt::make_self<AsyncRead>();
    winrt::com_ptr<DataView> view = winrt::make_self<DataView>();
    winrt::com_ptr<DragArgs> args = winrt::make_self<DragArgs>();
    explicit Request(const muxc::ScrollViewer& strip) {
        view->read = read.as<Read>();
        args->view = view.as<wdt::DataPackageView>();
        args->strip = strip;
    }
    mux::DragEventArgs Event() { return args.as<mux::DragEventArgs>(); }
};

inline ws::IStorageItem Folder(const std::wstring& path) { return winrt::make<Item>(path).as<ws::IStorageItem>(); }
inline ws::IStorageItem File(const std::wstring& path) { return winrt::make<Item>(path, false).as<ws::IStorageItem>(); }
}
