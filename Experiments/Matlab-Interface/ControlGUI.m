function varargout = ControlGUI(varargin)
% CONTROLGUI MATLAB code for ControlGUI.fig
%      CONTROLGUI, by itself, creates a new CONTROLGUI or raises the existing
%      singleton*.
%
%      H = CONTROLGUI returns the handle to a new CONTROLGUI or the handle to
%      the existing singleton*.
%
%      CONTROLGUI('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in CONTROLGUI.M with the given input arguments.
%
%      CONTROLGUI('Property','Value',...) creates a new CONTROLGUI or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before ControlGUI_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to ControlGUI_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help ControlGUI

% Last Modified by GUIDE v2.5 18-Jan-2018 15:02:42

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @ControlGUI_OpeningFcn, ...
                   'gui_OutputFcn',  @ControlGUI_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT

% --- Executes just before ControlGUI is made visible.
function ControlGUI_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to ControlGUI (see VARARGIN)

% Reset coms
instrreset

handles.s = serial('COM5');
s.Timeout = 1;
handles.duration = 2500;
handles.response = 0;
handles.input = 0;
handles.time = 0;
grid on

% Choose default command line output for ControlGUI
handles.output = hObject;

% Update handles structure
guidata(hObject, handles);

% UIWAIT makes ControlGUI wait for user response (see UIRESUME)
% uiwait(handles.figure1);


% --- Outputs from this function are returned to the command line.
function varargout = ControlGUI_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


% --- Executes on button press in run_button.
function run_button_Callback(hObject, eventdata, handles)
% hObject    handle to run_button (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
duration = str2num(handles.duration_edit.String);
type_label = handles.input_button_group.SelectedObject.String;
if(strcmp(type_label,'pHUp'))
    type = 21;
elseif(strcmp(type_label,'pHDown'))
    type = 22;
elseif(strcmp(type_label,'GerminationEC'))
    type = 23;
elseif(strcmp(type_label,'VegetationEC'))
    type = 24;
end

s = handles.s;
flushinput(s);
fwrite(s,1);    % Send message start byte
fwrite(s,type); % Send type of input
[lsb, msb] = split(duration);
fwrite(s,lsb);    % Send LSB (duration in ms)
fwrite(s,msb);    % Send MSB (duration in ms)

tic;
while(s.BytesAvailable() < 1)
    t = toc;
   if (t  > 1)
       return;
   end
end
ack = fread(s,1);

time = zeros(0,0);
input = zeros(0,0);
response = zeros(0,0);

% Begin listening for values from Arduino and plotting
while(true)
        if(s.BytesAvailable())
           identifier = fread(s,1);
           switch identifier
               case 'i'    % (105) Data (input active)
                   input(end+1) = 1;
                   lsb = fread(s,1);
                   msb = fread(s,1);
                   time(end+1) = 1e-3*combine(lsb,msb);
                   lsb = fread(s,1);
                   msb = fread(s,1);
                   response(end+1) = combine(lsb,msb);
               case 'n'    % (110) Data (no input active)
                   input(end+1) = 0;
                   lsb = fread(s,1);
                   msb = fread(s,1);
                   time(end+1) = 1e-3*combine(lsb,msb);                  
                   lsb = fread(s,1);
                   msb = fread(s,1);                   
                   response(end+1) = combine(lsb,msb);
               case 27 % Stop byte
                   break;
           end
        end
    plot(time,response);
    xlabel('Time, t[s]');
    ylabel('PH, pH[pH]');
    grid on
    drawnow;
end
handles.time = time;
handles.response = response;
handles.input = input;
% Update handles structure
guidata(hObject, handles);



function test_name_edit_Callback(hObject, eventdata, handles)
% hObject    handle to test_name_edit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of test_name_edit as text
%        str2double(get(hObject,'String')) returns contents of test_name_edit as a double


% --- Executes during object creation, after setting all properties.
function test_name_edit_CreateFcn(hObject, eventdata, handles)
% hObject    handle to test_name_edit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in save_button.
function save_button_Callback(hObject, eventdata, handles)
% hObject    handle to save_button (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
testname = handles.test_name_edit.String;
type = handles.input_button_group.SelectedObject.String;
probe_pos = handles.pos_button_group.SelectedObject.String;
duration = handles.duration_edit.String;
filename = strcat(testname,type, '-', probe_pos, '-', duration);
response = handles.response;
input = handles.input;
time = handles.time;
save(filename, 'response', 'input', 'time');

function duration_edit_Callback(hObject, eventdata, handles)
% hObject    handle to duration_edit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of duration_edit as text
%        str2double(get(hObject,'String')) returns contents of duration_edit as a double


% --- Executes during object creation, after setting all properties.
function duration_edit_CreateFcn(hObject, eventdata, handles)
% hObject    handle to duration_edit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in connect_button.
function connect_button_Callback(hObject, eventdata, handles)
% hObject    handle to connect_button (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of connect_button

% If not connected, connect to the current selected port and change button
% to the disconnect button. If it is connected, disconnect and change to
% connect button.
if(handles.connect_button.Value)
    try
        handles.s = serial(handles.com_edit.String);
        fopen(handles.s);
        handles.connect_button.String = 'Disconnect';
        handles.status_box.String = 'Connected';
        handles.com_edit.BackgroundColor = [1, 1, 1];
    catch
        handles.com_edit.BackgroundColor = [0.90, 0.90, 0.95];
        handles.connect_button.Value = 0;
    end
else
    fclose(handles.s);
    handles.connect_button.String = 'Connect';
    handles.status_box.String = 'Not Connected';
end
guidata(hObject, handles);


function com_edit_Callback(hObject, eventdata, handles)
% hObject    handle to com_edit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of com_edit as text
%        str2double(get(hObject,'String')) returns contents of com_edit as a double


% --- Executes during object creation, after setting all properties.
function com_edit_CreateFcn(hObject, eventdata, handles)
% hObject    handle to com_edit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end
