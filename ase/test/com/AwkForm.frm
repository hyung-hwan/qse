VERSION 5.00
Begin VB.Form AwkForm 
   Caption         =   "ASE.COM.AWK"
   ClientHeight    =   8100
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   12900
   LinkTopic       =   "AwkForm"
   MaxButton       =   0   'False
   ScaleHeight     =   8100
   ScaleWidth      =   12900
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton btnClearAll 
      Caption         =   "Clear All"
      Height          =   375
      Left            =   240
      TabIndex        =   14
      Top             =   4560
      Width           =   2295
   End
   Begin VB.CommandButton btnAddArgument 
      Caption         =   "Add"
      Height          =   375
      Left            =   1920
      TabIndex        =   12
      Top             =   4080
      Width           =   615
   End
   Begin VB.TextBox txtArgument 
      Height          =   375
      Left            =   240
      TabIndex        =   11
      Top             =   4080
      Width           =   1575
   End
   Begin VB.ListBox lstArguments 
      Height          =   2595
      Left            =   240
      TabIndex        =   10
      Top             =   1320
      Width           =   2295
   End
   Begin VB.ComboBox EntryPoint 
      Height          =   315
      ItemData        =   "AwkForm.frx":0000
      Left            =   240
      List            =   "AwkForm.frx":0007
      TabIndex        =   9
      Top             =   480
      Width           =   2295
   End
   Begin VB.TextBox ConsoleIn 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   3735
      Left            =   2760
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   2
      Top             =   4320
      Width           =   5055
   End
   Begin VB.TextBox SourceIn 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   3615
      Left            =   2760
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   0
      Top             =   360
      Width           =   5055
   End
   Begin VB.TextBox SourceOut 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   3615
      Left            =   7920
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   1
      Top             =   360
      Width           =   4935
   End
   Begin VB.CommandButton btnExecute 
      Caption         =   "Execute"
      Height          =   375
      Left            =   1440
      TabIndex        =   5
      Top             =   7680
      Width           =   1215
   End
   Begin VB.TextBox ConsoleOut 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   3735
      Left            =   7920
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   3
      Top             =   4320
      Width           =   4935
   End
   Begin VB.Frame Frame1 
      Caption         =   "Arguments"
      Height          =   4335
      Left            =   120
      TabIndex        =   13
      Top             =   1080
      Width           =   2535
      Begin VB.CheckBox chkPassToEntryPoint 
         Caption         =   "Pass To Entry Point"
         Height          =   255
         Left            =   360
         TabIndex        =   16
         Top             =   3960
         Width           =   1815
      End
   End
   Begin VB.Frame Frame2 
      Caption         =   "Entry Point"
      Height          =   855
      Left            =   120
      TabIndex        =   15
      Top             =   120
      Width           =   2535
   End
   Begin VB.Label Label4 
      Caption         =   "Console Out"
      Height          =   255
      Left            =   7920
      TabIndex        =   8
      Top             =   4080
      Width           =   3735
   End
   Begin VB.Label Label3 
      Caption         =   "Console In"
      Height          =   255
      Left            =   2760
      TabIndex        =   7
      Top             =   4080
      Width           =   3735
   End
   Begin VB.Label Label2 
      Caption         =   "Source Out"
      Height          =   255
      Left            =   7920
      TabIndex        =   6
      Top             =   120
      Width           =   3735
   End
   Begin VB.Label Label1 
      Caption         =   "Source In"
      Height          =   255
      Left            =   2760
      TabIndex        =   4
      Top             =   120
      Width           =   2415
   End
End
Attribute VB_Name = "AwkForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Option Base 0
Dim source_first As Boolean
Public WithEvents Awk As ASELib.Awk
Attribute Awk.VB_VarHelpID = -1

Private Sub btnAddArgument_Click()
    Dim arg As String
    
    arg = txtArgument.Text
    If Len(arg) > 0 Then
        lstArguments.AddItem (arg)
        txtArgument.Text = ""
        txtArgument.SetFocus
    End If
End Sub

Private Sub btnClearAll_Click()
    lstArguments.Clear
End Sub

Private Sub btnExecute_Click()

    source_first = True
    
    ConsoleOut.Text = ""
    SourceOut.Text = ""
    
    Set Awk = New ASELib.Awk
    
    Awk.ExplicitVariable = True
    Awk.ImplicitVariable = True
    Awk.UseCrlf = True
    Awk.IdivOperator = True
    Awk.ShiftOperators = True
    
    Awk.MaxDepthForBlockParse = 20
    Awk.MaxDepthForBlockRun = 30
    Awk.MaxDepthForExprParse = 20
    Awk.MaxDepthForExprRun = 30
    'Awk.MaxDepthForRexBuild = 10
    'Awk.MaxDepthForRexMatch = 10
    
    Awk.UseLongLong = False
    Awk.Debug = True
    
    If Not Awk.AddFunction("sin", 1, 1) Then
        MsgBox "Cannot add builtin function - " + Awk.ErrorMessage
        Exit Sub
    End If
    If Not Awk.AddFunction("cos", 1, 1) Then
        MsgBox "Cannot add builtin function - " + Awk.ErrorMessage
        Exit Sub
    End If
    Call Awk.AddFunction("tan", 1, 1)
    Call Awk.AddFunction("sqrt", 1, 1)
    Call Awk.AddFunction("trim", 1, 1)
    'Call Awk.DeleteFunction("tan")
    
    If Not Awk.Parse() Then
        MsgBox "PARSE ERROR [" + Str(Awk.ErrorLine) + "]" + Awk.ErrorMessage
    Else
        Dim n As Boolean
        
        Awk.EntryPoint = Trim(EntryPoint.Text)
        
        If lstArguments.ListCount = 0 Then
            n = Awk.Run(Null)
        Else
            ReDim Args(lstArguments.ListCount - 1) As String
            Dim i As Integer
            
            Awk.ArgumentsToEntryPoint = chkPassToEntryPoint.value
        
            For i = 0 To lstArguments.ListCount - 1
                Args(i) = lstArguments.List(i)
            Next i
            
            n = Awk.Run(Args)
        End If
        
        If Not n Then
            MsgBox "RUN ERROR [" + Str(Awk.ErrorLine) + "]" + Awk.ErrorMessage
        End If
    End If
    
    Set Awk = Nothing
    
End Sub

Function Awk_OpenSource(ByVal mode As ASELib.AwkSourceMode) As Long
    Awk_OpenSource = 1
End Function

Function Awk_CloseSource(ByVal mode As ASELib.AwkSourceMode) As Long
    Awk_CloseSource = 0
End Function

Function Awk_ReadSource(ByVal buf As ASELib.Buffer) As Long
    If source_first Then
        buf.value = SourceIn.Text
        Awk_ReadSource = Len(buf.value)
        source_first = False
    Else
        Awk_ReadSource = 0
    End If
End Function

Function Awk_WriteSource(ByVal buf As ASELib.Buffer) As Long
    Dim value As String
    Dim l As Integer
    
    value = buf.value
    l = Len(value)
    SourceOut.Text = SourceOut.Text + value
    Awk_WriteSource = Len(value)
End Function

Function Awk_OpenExtio(ByVal extio As ASELib.AwkExtio) As Long
    Awk_OpenExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_READ Then
            extio.Handle = New AwkExtioConsole
            With extio.Handle
                .EOF = False
            End With
            Awk_OpenExtio = 1
        ElseIf extio.mode = ASELib.AWK_EXTIO_CONSOLE_WRITE Then
            extio.Handle = New AwkExtioConsole
            With extio.Handle
                .EOF = False
            End With
            Awk_OpenExtio = 1
        End If
        
    Case ASELib.AWK_EXTIO_FILE
        If extio.mode = ASELib.AWK_EXTIO_FILE_READ Then
            extio.Handle = FreeFile
            On Error GoTo ErrorTrap
            Open extio.name For Input As #extio.Handle
            On Error GoTo 0
            Awk_OpenExtio = 1
        ElseIf extio.mode = ASELib.AWK_EXTIO_FILE_WRITE Then
            extio.Handle = FreeFile
            On Error GoTo ErrorTrap
            Open extio.name For Output As #extio.Handle
            On Error GoTo 0
            Awk_OpenExtio = 1
        ElseIf extio.mode = ASELib.AWK_EXTIO_FILE_APPEND Then
            extio.Handle = FreeFile
            On Error GoTo ErrorTrap
            Open extio.name For Append As #extio.Handle
            On Error GoTo 0
            Awk_OpenExtio = 1
        End If
        
    Case ASELib.AWK_EXTIO_PIPE
        Awk_OpenExtio = -1
    Case ASELib.AWK_EXTIO_COPROC
        Awk_OpenExtio = -1
    End Select
    
    Exit Function
    
ErrorTrap:
    Exit Function
End Function

Function Awk_CloseExtio(ByVal extio As ASELib.AwkExtio) As Long
    Awk_CloseExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_READ Or _
           extio.mode = ASELib.AWK_EXTIO_CONSOLE_WRITE Then
            extio.Handle = Nothing
            Awk_CloseExtio = 0
        End If
    Case ASELib.AWK_EXTIO_FILE
        If extio.mode = ASELib.AWK_EXTIO_FILE_READ Or _
           extio.mode = ASELib.AWK_EXTIO_FILE_WRITE Or _
           extio.mode = ASELib.AWK_EXTIO_FILE_APPEND Then
            Close #extio.Handle
            Awk_CloseExtio = 0
        End If
    Case ASELib.AWK_EXTIO_PIPE
        Awk_CloseExtio = -1
    Case ASELib.AWK_EXTIO_COPROC
        Awk_CloseExtio = -1
    End Select
    
End Function

Function Awk_ReadExtio(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Awk_ReadExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_READ Then
            Awk_ReadExtio = ReadExtioConsole(extio, buf)
        End If
        
    Case ASELib.AWK_EXTIO_FILE
        If extio.mode = ASELib.AWK_EXTIO_FILE_READ Then
            Awk_ReadExtio = ReadExtioFile(extio, buf)
        End If
        
    Case ASELib.AWK_EXTIO_PIPE
        Awk_ReadExtio = -1
    Case ASELib.AWK_EXTIO_COPROC
        Awk_ReadExtio = -1
    End Select
    
End Function

Function Awk_WriteExtio(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Awk_WriteExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_WRITE Then
            Awk_WriteExtio = WriteExtioConsole(extio, buf)
        End If
    Case ASELib.AWK_EXTIO_FILE
        If extio.mode = ASELib.AWK_EXTIO_FILE_WRITE Or _
           extio.mode = ASELib.AWK_EXTIO_FILE_APPEND Then
            Awk_WriteExtio = WriteExtioFile(extio, buf)
        End If
    Case ASELib.AWK_EXTIO_PIPE
        Awk_WriteExtio = -1
    Case ASELib.AWK_EXTIO_COPROC
        Awk_WriteExtio = -1
    End Select
End Function

Function ReadExtioConsole(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String

    If Not extio.Handle.EOF Then
        value = ConsoleIn.Text
        extio.Handle.EOF = True
        buf.value = value
        ReadExtioConsole = Len(value)
    Else
        ReadExtioConsole = 0
    End If
End Function

Function ReadExtioFile(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String
    
    If EOF(extio.Handle) Then
        ReadExtioFile = 0
        Exit Function
    End If
    
    On Error GoTo ErrorTrap
    Line Input #extio.Handle, value
    On Error GoTo 0
    
    value = value + vbCrLf
    
    buf.value = value
    ReadExtioFile = Len(buf.value)
    Exit Function
    
ErrorTrap:
    ReadExtioFile = -1
    Exit Function
End Function
    
Function WriteExtioConsole(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String
        
    value = buf.value
    ConsoleOut.Text = ConsoleOut.Text + value
    WriteExtioConsole = Len(value)
End Function

Function WriteExtioFile(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String
    
    WriteExtioFile = -1
    
    value = buf.value
    On Error GoTo ErrorTrap
    Print #extio.Handle, value;
    On Error GoTo 0
    WriteExtioFile = Len(value)
    
    Exit Function
    
ErrorTrap:
    Exit Function
End Function

Function Awk_HandleBuiltinFunction(ByVal name As String, ByVal Args As Variant) As Variant

    If name = "sin" Then
        If IsNull(Args(0)) Then
            Awk_HandleBuiltinFunction = Sin(0)
        ElseIf IsNumeric(Args(0)) Then
            Awk_HandleBuiltinFunction = Sin(Args(0))
        Else
            Awk_HandleBuiltinFunction = Sin(Val(Args(0)))
        End If
    ElseIf name = "cos" Then
        If TypeName(Args(0)) = "Long" Or TypeName(Args(0)) = "Double" Then
            Awk_HandleBuiltinFunction = Cos(Args(0))
        ElseIf TypeName(Args(0)) = "String" Then
            Awk_HandleBuiltinFunction = Cos(Val(Args(0)))
        ElseIf TypeName(Args(0)) = "Null" Then
            Awk_HandleBuiltinFunction = Cos(0)
        End If
    ElseIf name = "tan" Then
        If TypeName(Args(0)) = "Long" Or TypeName(Args(0)) = "Double" Then
            Awk_HandleBuiltinFunction = Tan(Args(0))
        ElseIf TypeName(Args(0)) = "String" Then
            Awk_HandleBuiltinFunction = Tan(Val(Args(0)))
        ElseIf TypeName(Args(0)) = "Null" Then
            Awk_HandleBuiltinFunction = Tan(0)
        End If
    ElseIf name = "sqrt" Then
        If IsNull(Args(0)) Then
            Awk_HandleBuiltinFunction = Sqr(0)
        ElseIf IsNumeric(Args(0)) Then
            Awk_HandleBuiltinFunction = Sqr(Args(0))
        Else
            Awk_HandleBuiltinFunction = Sqr(Val(Args(0)))
        End If
    ElseIf name = "trim" Then
        Awk_HandleBuiltinFunction = Trim(Args(0))
    End If
    
    'Dim i As Integer
    'Dim xxx As String

    'MsgBox name
    
    'For i = LBound(args) To UBound(args)
    '    xxx = xxx & "," & args(i)
    'Next i
    
    'MsgBox xxx
End Function


Private Sub Form_Load()
    SourceIn.Text = ""
    SourceOut.Text = ""
    ConsoleIn.Text = ""
    ConsoleOut.Text = ""
End Sub

