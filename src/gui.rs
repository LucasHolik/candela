// GUI module for Win32 slider window
use windows::{
    core::*,
    Win32::Foundation::*,
    Win32::Graphics::Gdi::*,
    Win32::System::LibraryLoader::GetModuleHandleW,
    Win32::UI::WindowsAndMessaging::*,
};

pub const IDC_BRIGHTNESS_SLIDER: i32 = 1001;

use windows::Win32::UI::WindowsAndMessaging::{SetWindowPos, ShowWindow, SW_HIDE, SWP_SHOWWINDOW, SWP_NOMOVE, SWP_NOSIZE, HWND_TOPMOST};
pub unsafe fn create_slider_window(x: i32, y: i32) -> Result<HWND> {
    let hinstance = GetModuleHandleW(None)?;
    
    // Register the window class
    let class_name = w!("CandelaSliderWndClass");
    let wc = WNDCLASSW {
        style: CS_HREDRAW | CS_VREDRAW,
        lpfnWndProc: Some(slider_wnd_proc),
        hInstance: hinstance.into(),
        hCursor: LoadCursorW(None, IDC_ARROW)?,
        hbrBackground: HBRUSH(COLOR_WINDOW.0 as isize),
        lpszClassName: class_name,
        ..Default::default()
    };
    
    RegisterClassW(&wc);
    
    // Create the window
    let hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        class_name,
        w!("Brightness"),
        WS_POPUP | WS_THICKFRAME,
        x,
        y,
        60,   // Width
        250,  // Height
        None,
        None,
        hinstance,
        None,
    );
    
    if hwnd.0 == 0 {
        return Err(ERROR_INVALID_WINDOW_HANDLE.into());
    }
    
    // Create the slider control (using the correct Windows API)
    let hslider = CreateWindowExW(
        Default::default(),
        w!("msctls_trackbar32"), // Windows trackbar class
        w!(""),
        WS_CHILD | WS_VISIBLE | WINDOW_STYLE(0x0002), // TBS_VERT (vertical)
        15,  // x
        10,  // y
        30,  // width
        200, // height
        hwnd,
        HMENU(IDC_BRIGHTNESS_SLIDER as isize),
        hinstance,
        None,
    );
    
    if hslider.0 != 0 {
        // Use raw message calls for trackbar-specific messages
        const TBM_SETRANGE: u32 = WM_USER + 6;
        const TBM_SETPOS: u32 = WM_USER + 5;
        const TBM_SETTICFREQ: u32 = WM_USER + 20;
        
        // Set slider range (0 to 100)
        SendMessageW(hslider, TBM_SETRANGE, WPARAM(1), LPARAM(0x00640000)); // MAKELONG(0, 100)
        SendMessageW(hslider, TBM_SETPOS, WPARAM(100), LPARAM(0)); // Set to 100 initially
        SendMessageW(hslider, TBM_SETTICFREQ, WPARAM(10), LPARAM(0)); // Tick every 10 units
    }
    
    // Set window position to be on top
    let _ = SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        x,
        y,
        60,
        250,
        SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE,
    );
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    Ok(hwnd)
}

unsafe extern "system" fn slider_wnd_proc(hwnd: HWND, msg: u32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    match msg {
        WM_HSCROLL | WM_VSCROLL => {
            const TB_THUMBTRACK: u16 = 5;
            const TB_ENDTRACK: u16 = 8;
            const TBM_GETPOS: u32 = WM_USER + 0;
            
            let scroll_code = (wparam.0 & 0xFFFF) as u16;
            if scroll_code == TB_THUMBTRACK || scroll_code == TB_ENDTRACK {
                let hwnd_track = HWND(lparam.0);
                if GetDlgCtrlID(hwnd_track) == IDC_BRIGHTNESS_SLIDER {
                    let pos = SendMessageW(hwnd_track, TBM_GETPOS, WPARAM(0), LPARAM(0));
                    let _brightness = pos.0 as u32; // Using underscore to indicate it's intentionally unused for now
                    
                    // Update brightness based on slider position
                    // In a real implementation, we'd call the brightness controller
                    // For now, just get the current app state and update brightness
                }
            }
            LRESULT(0)
        }
        WM_COMMAND => {
            let control_id = (wparam.0 & 0xFFFF) as i32;
            match control_id {
                val if val == IDC_BRIGHTNESS_SLIDER => {
                    // Handle slider specific commands if needed
                }
                _ => {}
            }
            LRESULT(0)
        }
        WM_KILLFOCUS => {
            // When the window loses focus, hide it
            ShowWindow(hwnd, SW_HIDE);
            // Reset the global slider window handle
            unsafe {
                crate::SLIDER_WINDOW = HWND(0);
            }
            LRESULT(0)
        }
        WM_ACTIVATE => {
            // If the window is being deactivated and it wasn't activated by the user clicking on it
            let activation_state = (wparam.0 & 0xFFFF) as u16;
            if activation_state == 0 { // WA_INACTIVE is 0
                // Hide the window when it loses focus
                ShowWindow(hwnd, SW_HIDE);
                // Reset the global slider window handle
                crate::SLIDER_WINDOW = HWND(0);
            }
            DefWindowProcW(hwnd, msg, wparam, lparam)
        }
        WM_DESTROY => {
            // Reset the global slider window handle when destroyed
            crate::SLIDER_WINDOW = HWND(0);
            PostQuitMessage(0);
            LRESULT(0)
        }
        WM_PAINT => {
            // Handle painting
            let mut ps = PAINTSTRUCT::default();
            let _hdc = BeginPaint(hwnd, &mut ps);
            EndPaint(hwnd, &ps);
            LRESULT(0)
        }
        _ => DefWindowProcW(hwnd, msg, wparam, lparam),
    }
}
